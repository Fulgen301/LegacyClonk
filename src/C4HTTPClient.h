/*
 * LegacyClonk
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
 * Copyright (c) 2023, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#pragma once

#include "C4Coroutine.h"
#include "C4CurlSystem.h"
#include "C4Network2.h"
#include "StdBuf.h"
#include "StdSync.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>

using CURLM = struct Curl_multi;
using CURL = struct Curl_easy;
using curl_socket_t = SOCKET;
using CURLU = struct Curl_URL;

class C4HTTPClient
{
public:
	using Headers = std::unordered_map<std::string_view, std::string_view>;
	using Notify = std::function<void(C4Network2HTTPClient *)>;
	using ProgressCallback = std::function<bool(std::int64_t, std::int64_t)>;

	class Exception : public C4CurlSystem::Exception
	{
		using C4CurlSystem::Exception::Exception;
	};

	class Uri
	{
	private:
		struct CURLUDeleter
		{
			void operator()(CURLU *const uri);
		};

	public:
		Uri(const std::string &serverAddress, std::uint16_t port = 0);

	public:
		std::string GetServerAddress() const;
		std::string GetUriAsString() const;

		auto get() const { return uri.get(); }

	private:
		std::unique_ptr<CURLU, CURLUDeleter> uri;
	};

	struct Request
	{
		C4HTTPClient::Uri Uri;
		bool Binary{false};
		std::span<const std::byte> Data;
	};

	struct Result
	{
		StdBuf Buffer;
		C4NetIO::addr_t ServerAddress;
	};

public:
	C4HTTPClient() = default;

public:
	C4Task::Hot<Result> GetAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});
	C4Task::Hot<Result> PostAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});

private:
	C4Task::Hot<Result> RequestAsync(Request request, ProgressCallback progressCallback, Headers headers, bool post);
	C4CurlSystem::EasyHandle PrepareRequest(const Request &request, Headers headers, bool post);

	static std::size_t WriteFunction(char *ptr, std::size_t, std::size_t nmemb, void *userData);
	static int XferInfoFunction(void *userData, std::int64_t downloadTotal, std::int64_t downloadNow, std::int64_t, std::int64_t);

private:
	std::uint16_t defaultPort{0};
};