/// \file     Session.hpp
/// \authors  hsoszynski
/// \version  1.0
/// \date     05/07/18
/// \license  GPLv3
/// \brief    Copyright (c) 2018 Advens. All rights reserved.

#pragma once

#include <memory>
#include "config.hpp"
#include "protocol.h"
#include "Generator.hpp"
#include "Manager.hpp"
#include "../../toolkit/lru_cache.hpp"
#include "../../toolkit/xxhash.h"
#include "../../toolkit/xxhash.hpp"
#include "../../toolkit/rapidjson/document.h"
#include "../../toolkit/rapidjson/writer.h"
#include "../../toolkit/rapidjson/stringbuffer.h"
#include "Time.hpp"

#include "ASession.fwd.hpp"

#define DARWIN_SESSION_BUFFER_SIZE 2048
#define DARWIN_DEFAULT_THRESHOLD 80
#define DARWIN_ERROR_RETURN 101

namespace darwin {

    class ASession : public std::enable_shared_from_this<ASession> {
    public:
        ASession(Manager& manager,
                Generator& generator);

        virtual ~ASession() = default;

        // Make the Session non copyable & non movable
        ASession(ASession const&) = delete;

        ASession(ASession const&&) = delete;

        ASession& operator=(ASession const&) = delete;

        ASession& operator=(ASession const&&) = delete;

    public:
        /// Start the session and the async read of the incoming packet.
        virtual void Start();

        /// Stop the session and close the socket.
        virtual void Stop() = 0;

        /// Set the filter's threshold
        ///
        /// \param threshold The threshold wanted.
        virtual void SetThreshold(std::size_t const& threshold) final;

        /// Set the output's type of the filter
        ///
        /// \param name the string that represent the output type
        virtual void SetOutputType(std::string const& output) final;

        /// Get the filter's output type
        ///
        /// \return The filter's output type
        config::output_type GetOutputType();

        /// Get the filter's result in a log form
        ///
        /// \return The filter's log
        virtual std::string GetLogs();

        /// Get the filter's threshold
        ///
        /// \return The filter's threshold
        size_t GetThreshold();

        /// Transform the evt id in the header into a string
        ///
        /// \return evt_di as string
        std::string Evt_idToString();

    protected:
        

        /// Get the data to send to the next filter
        /// according to the filter's output type
        ///
        /// \param size data's size
        /// \param data data to send
        std::string GetDataToSendToFilter();

        virtual void WriteToClient(darwin_filter_packet_t* packet, size_t packet_size) = 0;

        virtual bool ConnectToNextFilter() = 0;

        virtual void WriteToFilter(darwin_filter_packet_t* packet, size_t packet_size) = 0;

        virtual void CloseFilterConnection() = 0;


        /// Send
        virtual void SendNext() final;

        /// Send result to the client.
        ///
        /// \return false if the function could not send the data, true otherwise.
        virtual bool SendToClient() noexcept;

        /// Send result to next filter.
        ///
        /// \return false if the function could not send the data, true otherwise.
        virtual bool SendToFilter() noexcept;

        /// Called when data is sent using Send() method.
        /// Terminate the session on failure.
        ///
        /// \param size The number of byte sent.
        virtual void
        SendToClientCallback(const boost::system::error_code& e,
                     std::size_t size);

        /// Called when data is sent using SendToFilter() method.
        /// Terminate the filter session on failure.
        ///
        /// \param size The number of byte sent.
        virtual void
        SendToFilterCallback(const boost::system::error_code& e,
                             std::size_t size);


        std::string JsonStringify(rapidjson::Document &json);

        bool PreParseBody();

        /// Set the async read for the header.
        ///
        /// \return -1 on error, 0 on socket closed & sizeof(header) on success.
        virtual void ReadHeader() = 0;

        /// Callback of async read for the header.
        /// Terminate the session on failure.
        virtual void ReadHeaderCallback(const boost::system::error_code& e, std::size_t size) final;

        /// Set the async read for the body.
        ///
        /// \return -1 on error, 0 on socket closed & sizeof(header) on success.
        virtual void ReadBody(std::size_t size) = 0;

        /// Callback of async read for the body.
        /// Terminate the session on failure.
        virtual void
        ReadBodyCallback(const boost::system::error_code& e,
                         std::size_t size) final;

        /// Execute the filter and
        virtual void ExecuteFilter() final;

        /// Sends a response with a body containing an error message
        ///
        /// \param message The error message to send
        /// \param code The error code to send
        virtual void SendErrorResponse(const std::string& message, const unsigned int code) final;

        friend std::shared_ptr<darwin::ATask> Generator::CreateTask(std::shared_ptr<ASession> s) noexcept;
        // Not accessible by children
    private:
        std::string _filter_name; //!< name of the filter
        config::output_type _output; //!< The filter's output.

        std::size_t _threshold = DARWIN_DEFAULT_THRESHOLD;

        // Accessible by children
    protected:
        std::array<char, DARWIN_SESSION_BUFFER_SIZE> _buffer; //!< Reading buffer for the body.
        Manager& _manager; //!< The associated connection manager.
        Generator& _generator; //!< The Task Generator.
        darwin_filter_packet_t _header; //!< Header received from the session.
        rapidjson::Document _body; //!< Body received from session (if any).
        std::string _raw_body; //!< Body received from session (if any), that will not be parsed.
        std::string _logs; //!< Represents data given in the logs by the Session
        std::string _response_body; //!< The body to send back to the client
        std::vector<unsigned int> _certitudes;
        bool _has_next_filter;
    };
}
