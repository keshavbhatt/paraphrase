#pragma once

// A tiny single-threaded HTTP/1.1 server for download tests, from Omnidict's tests: serves
// fixed bodies, honours `Range: bytes=N-` on the routes that opt in, can redirect, and can
// pause mid-body. Test code only, never in the daemon: it listens on 127.0.0.1 alone.

#include <QHash>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

class TestHttpServer : public QObject
{
    Q_OBJECT

public:
    struct Route
    {
        QByteArray body{};
        bool supportsRange = false;
        qsizetype splitAt = -1; ///< with pauseMs: send this much, wait, send the rest
        int pauseMs = 0;
        QString redirectTo{}; ///< answer 302 to this path instead
    };

    explicit TestHttpServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, &TestHttpServer::acceptConnections);
        const bool listening = m_server.listen(QHostAddress::LocalHost);
        Q_ASSERT(listening);
    }

    void addRoute(const QString& path, const Route& route) { m_routes.insert(path, route); }
    [[nodiscard]] bool sawRangeRequest(const QString& path) const { return m_rangeSeen.value(path, false); }
    [[nodiscard]] int requests(const QString& path) const { return m_requests.value(path, 0); }

    [[nodiscard]] QUrl urlFor(const QString& path) const
    {
        QUrl url;
        url.setScheme(QStringLiteral("http"));
        url.setHost(QStringLiteral("127.0.0.1"));
        url.setPort(m_server.serverPort());
        url.setPath(path);
        return url;
    }

private Q_SLOTS:
    void acceptConnections()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket* socket = m_server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] { handleReadyRead(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

private:
    void handleReadyRead(QTcpSocket* socket)
    {
        QByteArray& buffer = m_buffers[socket];
        buffer += socket->readAll();
        const qsizetype headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const QByteArray header = buffer.left(headerEnd);
        m_buffers.remove(socket);
        const QList<QByteArray> lines = header.split('\n');
        QString path;
        if (const QList<QByteArray> first = lines.first().trimmed().split(' '); first.size() >= 2) {
            path = QString::fromLatin1(first.at(1));
        }
        m_requests[path] += 1;
        QByteArray range;
        for (const QByteArray& line : lines) {
            if (line.toLower().startsWith("range:")) {
                range = line.mid(line.indexOf(':') + 1).trimmed();
            }
        }
        if (!m_routes.contains(path)) {
            writeHead(socket, "404 Not Found", 0, {});
            finish(socket);
            return;
        }
        const Route route = m_routes.value(path);
        if (!route.redirectTo.isEmpty()) {
            writeHead(socket, "302 Found", 0, "Location: " + urlFor(route.redirectTo).toEncoded() + "\r\n");
            finish(socket);
            return;
        }
        qint64 start = 0;
        bool partial = false;
        if (route.supportsRange && range.startsWith("bytes=")) {
            m_rangeSeen[path] = true;
            start = range.mid(6, range.indexOf('-') - 6).toLongLong();
            partial = true;
        }
        const QByteArray content = route.body.mid(start);
        QByteArray extra;
        if (partial) {
            extra = "Content-Range: bytes " + QByteArray::number(start) + '-' +
                    QByteArray::number(route.body.size() - 1) + '/' + QByteArray::number(route.body.size()) +
                    "\r\n";
        }
        writeHead(socket, partial ? "206 Partial Content" : "200 OK", content.size(), extra);
        if (route.splitAt > 0 && route.splitAt < content.size() && route.pauseMs > 0) {
            socket->write(content.left(route.splitAt));
            socket->flush();
            const QByteArray rest = content.mid(route.splitAt);
            const QPointer<QTcpSocket> guarded(socket);
            QTimer::singleShot(route.pauseMs, socket, [guarded, rest] {
                if (QTcpSocket* raw = guarded.data(); raw != nullptr) {
                    raw->write(rest);
                    finish(raw);
                }
            });
            return;
        }
        socket->write(content);
        finish(socket);
    }

    static void writeHead(QTcpSocket* socket, const QByteArray& status, qsizetype length,
                          const QByteArray& extra)
    {
        socket->write("HTTP/1.1 " + status + "\r\nContent-Length: " + QByteArray::number(length) + "\r\n" +
                      extra + "Connection: close\r\n\r\n");
    }

    static void finish(QTcpSocket* socket)
    {
        if (socket == nullptr) {
            return;
        }
        socket->flush();
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    QHash<QString, Route> m_routes;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QHash<QString, bool> m_rangeSeen;
    QHash<QString, int> m_requests;
};
