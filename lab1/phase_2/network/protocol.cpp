#include "protocol.h"

std::string Protocol::createRequest(RequestType type, const json &data, const std::string &token)
{
    json request;
    request["type"] = requestTypeToString(type);
    request["data"] = data;

    if (!token.empty())
    {
        request["token"] = token;
    }

    return request.dump();
}

bool Protocol::parseRequest(const std::string &requestStr, RequestType &type, json &data, std::string &token)
{
    try
    {
        json request = json::parse(requestStr);

        if (!request.contains("type") || !request.contains("data"))
        {
            return false;
        }

        type = stringToRequestType(request["type"]);
        data = request["data"];

        if (request.contains("token"))
        {
            token = request["token"];
        }
        else
        {
            token = "";
        }

        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

std::string Protocol::createResponse(ResponseStatus status, const json &data, const std::string &message)
{
    json response;
    response["status"] = responseStatusToString(status);
    response["data"] = data;

    if (!message.empty())
    {
        response["message"] = message;
    }

    return response.dump();
}

bool Protocol::parseResponse(const std::string &responseStr, ResponseStatus &status, json &data, std::string &message)
{
    try
    {
        json response = json::parse(responseStr);

        if (!response.contains("status") || !response.contains("data"))
        {
            return false;
        }

        status = stringToResponseStatus(response["status"]);
        data = response["data"];

        if (response.contains("message"))
        {
            message = response["message"];
        }
        else
        {
            message = "";
        }

        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

std::string Protocol::requestTypeToString(RequestType type)
{
    switch (type)
    {
    case RequestType::LOGIN:
        return "LOGIN";
    case RequestType::REGISTER:
        return "REGISTER";
    case RequestType::PRODUCT_LIST:
        return "PRODUCT_LIST";
    case RequestType::PRODUCT_SEARCH:
        return "PRODUCT_SEARCH";
    case RequestType::PRODUCT_DETAIL:
        return "PRODUCT_DETAIL";
    case RequestType::CART_ADD:
        return "CART_ADD";
    case RequestType::CART_REMOVE:
        return "CART_REMOVE";
    case RequestType::CART_UPDATE:
        return "CART_UPDATE";
    case RequestType::CART_LIST:
        return "CART_LIST";
    case RequestType::ORDER_CREATE:
        return "ORDER_CREATE";
    case RequestType::ORDER_LIST:
        return "ORDER_LIST";
    case RequestType::ORDER_DETAIL:
        return "ORDER_DETAIL";
    case RequestType::ORDER_PAY:
        return "ORDER_PAY";
    case RequestType::ORDER_CANCEL:
        return "ORDER_CANCEL";
    case RequestType::ORDER_STATUS:
        return "ORDER_STATUS";
    case RequestType::LOGOUT:
        return "LOGOUT";
    default:
        return "UNKNOWN";
    }
}

RequestType Protocol::stringToRequestType(const std::string &typeStr)
{
    if (typeStr == "LOGIN")
        return RequestType::LOGIN;
    if (typeStr == "REGISTER")
        return RequestType::REGISTER;
    if (typeStr == "PRODUCT_LIST")
        return RequestType::PRODUCT_LIST;
    if (typeStr == "PRODUCT_SEARCH")
        return RequestType::PRODUCT_SEARCH;
    if (typeStr == "PRODUCT_DETAIL")
        return RequestType::PRODUCT_DETAIL;
    if (typeStr == "CART_ADD")
        return RequestType::CART_ADD;
    if (typeStr == "CART_REMOVE")
        return RequestType::CART_REMOVE;
    if (typeStr == "CART_UPDATE")
        return RequestType::CART_UPDATE;
    if (typeStr == "CART_LIST")
        return RequestType::CART_LIST;
    if (typeStr == "ORDER_CREATE")
        return RequestType::ORDER_CREATE;
    if (typeStr == "ORDER_LIST")
        return RequestType::ORDER_LIST;
    if (typeStr == "ORDER_DETAIL")
        return RequestType::ORDER_DETAIL;
    if (typeStr == "ORDER_PAY")
        return RequestType::ORDER_PAY;
    if (typeStr == "ORDER_CANCEL")
        return RequestType::ORDER_CANCEL;
    if (typeStr == "ORDER_STATUS")
        return RequestType::ORDER_STATUS;
    if (typeStr == "LOGOUT")
        return RequestType::LOGOUT;

    // 默认值，实际上应该抛出异常或有其他错误处理
    return RequestType::LOGIN;
}

std::string Protocol::responseStatusToString(ResponseStatus status)
{
    switch (status)
    {
    case ResponseStatus::SUCCESS:
        return "SUCCESS";
    case ResponseStatus::FAILED:
        return "FAILED";
    case ResponseStatus::UNAUTHORIZED:
        return "UNAUTHORIZED";
    case ResponseStatus::NOT_FOUND:
        return "NOT_FOUND";
    case ResponseStatus::BAD_REQUEST:
        return "BAD_REQUEST";
    case ResponseStatus::SERVER_ERROR:
        return "SERVER_ERROR";
    default:
        return "UNKNOWN";
    }
}

ResponseStatus Protocol::stringToResponseStatus(const std::string &statusStr)
{
    if (statusStr == "SUCCESS")
        return ResponseStatus::SUCCESS;
    if (statusStr == "FAILED")
        return ResponseStatus::FAILED;
    if (statusStr == "UNAUTHORIZED")
        return ResponseStatus::UNAUTHORIZED;
    if (statusStr == "NOT_FOUND")
        return ResponseStatus::NOT_FOUND;
    if (statusStr == "BAD_REQUEST")
        return ResponseStatus::BAD_REQUEST;
    if (statusStr == "SERVER_ERROR")
        return ResponseStatus::SERVER_ERROR;

    // 默认值，实际上应该抛出异常或有其他错误处理
    return ResponseStatus::FAILED;
}
