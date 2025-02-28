/*
 * Copyright (c) 2021, Ali Mohammad Pur <mpfard@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ConnectionCache.h"
#include <LibCore/EventLoop.h>

namespace RequestServer::ConnectionCache {

HashMap<ConnectionKey, Vector<Connection<Core::TCPSocket>>> g_tcp_connection_cache {};
HashMap<ConnectionKey, Vector<Connection<TLS::TLSv12>>> g_tls_connection_cache {};

void request_did_finish(URL const& url, Core::Socket const* socket)
{
    if (!socket) {
        dbgln("Request with a null socket finished for URL {}", url);
        return;
    }

    dbgln("Request for {} finished", url);

    ConnectionKey key { url.host(), url.port_or_default() };
    auto fire_off_next_job = [&](auto& cache) {
        auto it = cache.find(key);
        if (it == cache.end()) {
            dbgln("Request for URL {} finished, but we don't own that!", url);
            return;
        }
        auto connection_it = it->value.find_if([&](auto& connection) { return connection.socket == socket; });
        if (connection_it.is_end()) {
            dbgln("Request for URL {} finished, but we don't have a socket for that!", url);
            return;
        }

        auto& connection = *connection_it;
        if (connection.request_queue.is_empty()) {
            connection.has_started = false;
            connection.removal_timer->on_timeout = [&connection, &cache_entry = it->value] {
                Core::deferred_invoke([&] {
                    dbgln("Removing no-longer-used connection {}", &connection);
                    cache_entry.remove_first_matching([&](auto& entry) { return &entry == &connection; });
                });
            };
            connection.removal_timer->start();
        } else {
            using SocketType = RemoveCVReference<decltype(*connection.socket)>;
            bool is_connected;
            if constexpr (IsSame<SocketType, TLS::TLSv12>)
                is_connected = connection.socket->is_established();
            else
                is_connected = connection.socket->is_connected();
            if (!is_connected) {
                // Create another socket for the connection.
                dbgln("Creating a new socket for {}", url);
                connection.socket = SocketType::construct(nullptr);
            }
            dbgln("Running next job in queue for connection {}", &connection);
            auto request = connection.request_queue.take_first();
            request(connection.socket);
        }
    };

    if (is<TLS::TLSv12>(socket))
        fire_off_next_job(g_tls_connection_cache);
    else if (is<Core::TCPSocket>(socket))
        fire_off_next_job(g_tcp_connection_cache);
    else
        dbgln("Unknown socket {} finished for URL {}", *socket, url);
}

}
