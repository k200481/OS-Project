#pragma once

namespace FS_IPC
{
    constexpr int MaxParamSize = sizeof(int) * 4;
    constexpr int regq_key = 12345;
    constexpr int regq_permissions = 0666;

    struct RequestBuf
    {
        long mtype;
        int pid;
        int uid;
        int qid;
        int shmid;
    };

    enum class Type : long
    {
        Open = 1,
        Close,
        Read,
        Write,
        Create,
        Remove,
        List,
        Exit,
    };

    struct CommandBuf
    {
        Type mtype;
        char params[MaxParamSize];
    };

    struct ReturnBuf
    {
        long mtype;
        int retval;
    };

    struct OpenParameters
    {
        int path_shmid;
    };

    struct CloseParameters
    {
        int f_idx;
    };

    struct ReadParameters
    {
        int f_idx;
        int size;
        int buf_shmid;
    };

    struct WriteParameters
    {
        int f_idx;
        int size;
        int buf_shmid;
    };

    struct CreateParameters
    {
        int f_idx;
        char etype;
        int name_shmid;
        int permissions;
    };

    struct RemoveParameters
    {
        int f_idx;
        int name_shmid;
    };

    struct ListParameters
    {
        int f_idx;
        int size;
        int listing_shmid;
    };

    struct ExitParameters
    {
        // literally empty
    };
}
