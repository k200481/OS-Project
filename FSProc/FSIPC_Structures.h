#pragma once

namespace FSIPC
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
        ErrorInfo
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
        char etype;
        int path_shmid;
        int permissions;
    };

    struct RemoveParameters
    {
        int path_shmid;
    };

    struct ListParameters
    {
        int f_idx;
        int size;
        int listing_shmid;
    };

    struct ErrorInfoParameters
    {
        int buf_shmid;
        int buf_size;
    };

    struct ExitParameters
    {
        // literally empty
    };
}
