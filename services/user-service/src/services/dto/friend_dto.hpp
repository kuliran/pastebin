#pragma once

#include <vector>
#include <string>

namespace user_service::dto {
    using GetFriendsResult = std::vector<std::string>;
    enum class GetFriendsError { kDbError };

    struct AddFriendResult{};
    enum class AddFriendError { kInvalidInput, kFriendLimitReached, kDbError };
    struct RmFriendResult{};

    enum class AreFriendsError { kDbError };
}