#include "hls_transcode.h"
#include "Mysql.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "TCPKernel.h"
using namespace std;

HLSTranscode* HLSTranscode::m_instance = NULL;

HLSTranscode* HLSTranscode::GetInstance() {
    if(m_instance == NULL) {
        m_instance = new HLSTranscode();
    }
    return m_instance;
}

HLSTranscode::HLSTranscode() {
    m_baseDir = "./hls";
    m_isRunning = false;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
}

HLSTranscode::~HLSTranscode() {
    m_isRunning = false;
    pthread_cond_signal(&m_cond);
    if(m_thread != 0) {
        pthread_join(m_thread, NULL);
    }
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

void HLSTranscode::Init() {
    if(m_isRunning) return;

    // 创建基础目录
    struct stat st = {0};
    if(stat(m_baseDir.c_str(), &st) == -1) {
        mkdir(m_baseDir.c_str(), 0777);
    }

    m_isRunning = true;
    pthread_create(&m_thread, NULL, TranscodeThread, this);
}

void HLSTranscode::AddTask(int fileid, const string& md5, const string& input_path) {
    pthread_mutex_lock(&m_mutex);

    TranscodeTask task;
    task.fileid = fileid;
    task.md5 = md5;
    task.input_path = input_path;
    task.output_dir = m_baseDir + "/" + md5;

    // 先查询，避免重复插入
    extern CMysql* g_sql;
    bool need_insert = true;

    if(g_sql) {
        // 先按fileid查
        int status;
        string hls_path, md5_check;
        if(g_sql->GetVideoTranscode(fileid, status, hls_path, md5_check)) {
            cout << "AddTask: already exists for fileid " << fileid << ", skipping insert" << endl;
            need_insert = false;
        } else {
            // 再按MD5查
            int fileid_from_md5;
            if(g_sql->GetVideoTranscodeByMD5(md5.c_str(), fileid_from_md5, status, hls_path)) {
                cout << "AddTask: already exists for MD5 " << md5 << ", skipping insert" << endl;
                need_insert = false;
            }
        }
    }

    if(need_insert) {
        m_taskQueue.push(task);

        // 插入数据库记录，状态为转码中
        if(g_sql) {
            g_sql->InsertVideoTranscode(fileid, md5.c_str(), transcode_processing, task.output_dir.c_str());
        }

        pthread_cond_signal(&m_cond);
    }

    pthread_mutex_unlock(&m_mutex);
}

bool HLSTranscode::IsVideoFile(const string& filename) {
    // 获取扩展名
    size_t dot_pos = filename.rfind('.');
    if(dot_pos == string::npos) return false;

    string ext = filename.substr(dot_pos + 1);
    // 转换为小写
    for(char& c : ext) {
        c = tolower(c);
    }

    // 支持的视频格式
    static const char* video_exts[] = {
        "mp4", "avi", "mkv", "flv", "rmvb", "mov", "wmv", "ts", "m4v", "webm", NULL
    };

    for(int i = 0; video_exts[i]; i++) {
        if(ext == video_exts[i]) {
            return true;
        }
    }

    return false;
}

bool HLSTranscode::IsAudioFile(const string& filename) {
    // 获取扩展名
    size_t dot_pos = filename.rfind('.');
    if(dot_pos == string::npos) return false;

    string ext = filename.substr(dot_pos + 1);
    // 转换为小写
    for(char& c : ext) {
        c = tolower(c);
    }

    // 支持的音频格式
    static const char* audio_exts[] = {
        "mp3", "wav", "flac", "aac", "ogg", "m4a", "wma", "ape", "alac", NULL
    };

    for(int i = 0; audio_exts[i]; i++) {
        if(ext == audio_exts[i]) {
            return true;
        }
    }

    return false;
}

int HLSTranscode::exec_cmd(const char* cmd) {
    // 使用 fork + execvp 完全替代 system()，避免信号干扰
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // 子进程

        // 重定向 stdin/stdout/stderr 到 /dev/null
        int devnull = open("/dev/null", O_RDWR);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) {
                close(devnull);
            }
        }

        // 使用 execl 执行命令
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);

        // 如果 exec 失败
        perror("execl failed");
        exit(127);
    } else {
        // 父进程，等待子进程结束
        int status;
        pid_t wait_pid = waitpid(pid, &status, 0);

        if (wait_pid == -1) {
            perror("waitpid failed");
            return -1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }

        return -1;
    }
}

void* HLSTranscode::TranscodeThread(void* arg) {
    HLSTranscode* pThis = (HLSTranscode*)arg;

    while(pThis->m_isRunning) {
        pthread_mutex_lock(&pThis->m_mutex);

        // 等待任务
        while(pThis->m_taskQueue.empty() && pThis->m_isRunning) {
            pthread_cond_wait(&pThis->m_cond, &pThis->m_mutex);
        }

        if(!pThis->m_isRunning) {
            pthread_mutex_unlock(&pThis->m_mutex);
            break;
        }

        // 取出任务
        TranscodeTask task = pThis->m_taskQueue.front();
        pThis->m_taskQueue.pop();

        pthread_mutex_unlock(&pThis->m_mutex);

        // 处理任务
        pThis->ProcessTask(task);
    }

    return NULL;
}

void HLSTranscode::ProcessTask(const TranscodeTask& task) {
    cout << "开始转码: fileid=" << task.fileid << ", md5=" << task.md5 << endl;

    // 创建输出目录
    struct stat st = {0};
    if(stat(task.output_dir.c_str(), &st) == -1) {
        mkdir(task.output_dir.c_str(), 0777);
    }

    string output_m3u8 = task.output_dir + "/output.m3u8";

    // 构建转码命令
    char cmd[4096] = {0};
    // 先尝试直接复制流（如果编码兼容）
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -i \"%s\" -c:v copy -c:a copy -start_number 0 -hls_time 10 -hls_list_size 0 -hls_segment_filename \"%s/output%%d.ts\" \"%s\" 2>/dev/null",
        task.input_path.c_str(), task.output_dir.c_str(), output_m3u8.c_str());

    int ret = exec_cmd(cmd);
    if(ret != 0) {
        // 复制流失败，重新编码为H.264+AAC
        cout << "复制流失败，开始重新编码..." << endl;
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -y -i \"%s\" -c:v libx264 -c:a aac -start_number 0 -hls_time 10 -hls_list_size 0 -hls_segment_filename \"%s/output%%d.ts\" \"%s\" 2>/dev/null",
            task.input_path.c_str(), task.output_dir.c_str(), output_m3u8.c_str());

        ret = exec_cmd(cmd);
    }

    // 更新数据库
    extern CMysql* g_sql;
    if(g_sql) {
        if(ret == 0) {
            cout << "转码成功: " << output_m3u8 << endl;
            g_sql->UpdateVideoTranscodeStatus(task.fileid, transcode_done, output_m3u8.c_str());
        } else {
            cout << "转码失败: fileid=" << task.fileid << endl;
            g_sql->UpdateVideoTranscodeStatus(task.fileid, transcode_failed);
        }
    }
}
