#ifndef HLS_TRANSCODE_H
#define HLS_TRANSCODE_H

#include "packdef.h"
#include <string>
#include <queue>
#include <pthread.h>

using namespace std;

// 转码任务结构
struct TranscodeTask {
    int fileid;
    string md5;
    string input_path;
    string output_dir;
};

class HLSTranscode {
public:
    static HLSTranscode* GetInstance();

    // 初始化
    void Init();

    // 添加转码任务
    void AddTask(int fileid, const string& md5, const string& input_path);

    // 检查是否为视频文件（根据扩展名）
    static bool IsVideoFile(const string& filename);
    // 检查是否为音频文件（根据扩展名）
    static bool IsAudioFile(const string& filename);

private:
    // 安全执行命令的辅助函数
    int exec_cmd(const char* cmd);
    // 获取文件的MD5
    static string GetFileMD5(const string& filepath);

    // 转码线程
    static void* TranscodeThread(void* arg);

    // 执行转码处理函数
    void ProcessTask(const TranscodeTask& task);

    // 创建输出目录
    string m_baseDir;

    // 任务队列
    queue<TranscodeTask> m_taskQueue;

    // 线程相关
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    pthread_t m_thread;
    bool m_isRunning;

    // 单例
    static HLSTranscode* m_instance;

    HLSTranscode();
    ~HLSTranscode();
    HLSTranscode(const HLSTranscode&);
    HLSTranscode& operator=(const HLSTranscode&);
};

#endif // HLS_TRANSCODE_H
