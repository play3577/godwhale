#ifndef GODWHALE_CLIENT_CLIENT_H
#define GODWHALE_CLIENT_CLIENT_H

#include "rsiservice.h" // IRSIListener
#include "replypacket.h"
#include "position.h"

namespace godwhale {

class CommandPacket;

namespace client {

/**
 * @brief クライアントに割り当てられた探索タスクを管理します。
 */
struct SearchTask
{
    explicit SearchTask()
        : iterationDepth(-1), plyDepth(0), move(0)
    {
    }

    explicit SearchTask(int itd, int pld, Move mv)
        : iterationDepth(itd), plyDepth(pld), move(mv)
    {
    }

    bool isIdle() const
    {
        return (iterationDepth < 0);
    }

    int getSearchDepth() const
    {
        return 1;
    }

    int iterationDepth;
    int plyDepth;
    Move move;
};


/**
 * @brief クライアントの実行・管理を行います。
 *
 * シングルトンクラスです。
 */
class Client : public enable_shared_from_this<Client>,
               public IRSIListener,
               private boost::noncopyable
{
public:
    /**
     * @brief 初期化処理を行います。
     */
    static int initializeInstance(int argc, char *argv[]);

    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<Client> get()
    {
        return ms_instance;
    }

public:
    ~Client();

    /**
     * @brief 正常にログイン処理が行われたかどうかを取得します。
     */
    bool isLogined() const
    {
        return m_logined;
    }

    /**
     * @brief クライアントの識別IDを取得します。
     */
    std::string const &getLoginId() const
    {
        return m_loginId;
    }

    /**
     * @brief 各クライアントの使用スレッド数を取得します。
     */
    int getThreadCount() const
    {
        return m_nthreads;
    }

    /**
     * @brief 思考中の局面IDを取得します。
     */
    int getPositionId() const
    {
        return m_positionId;
    }

    /**
     * @brief 探索ノード数を取得します。
     */
    long getNodeCount() const
    {
        return m_nodes;
    }

    void connect(std::string const & address, std::string const & port);
    void close();

    void login(std::string const & loginId);
    void sendReply(shared_ptr<ReplyPacket> reply, bool isOutLog = true);

    void addCommandFromRSI(std::string const & rsi);
    int mainloop();

private:
    static shared_ptr<Client> ms_instance;

    explicit Client();
    void initialize();
    void serviceThreadMain();

    virtual void onDisconnected();
    virtual void onRSIReceived(std::string const & command);

    void addCommand(shared_ptr<CommandPacket> command);
    void removeCommand(shared_ptr<CommandPacket> command);
    shared_ptr<CommandPacket> getNextCommand();

private:
    /* client_mainloop.cpp */

private:
    /* client_proce.cpp */
    int proce(bool nested);
    int proce_Login(shared_ptr<CommandPacket> command);
    int proce_SetPosition(shared_ptr<CommandPacket> command);
    int proce_MakeRootMove(shared_ptr<CommandPacket> command);

private:
    boost::asio::io_service m_service;
    shared_ptr<RSIService> m_rsiService;

    shared_ptr<boost::thread> m_serviceThread;
    volatile bool m_isAlive;

    mutable Mutex m_commandGuard;
    std::list<shared_ptr<CommandPacket>> m_commandList;

    bool m_logined;
    std::string m_loginId;
    int m_nthreads;

    int m_positionId;
    Position m_position; ///< ルート局面です。

    long m_nodes;
};

} // namespace client
} // namespace godwhale

#endif
