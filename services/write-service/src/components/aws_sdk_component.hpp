#pragma once
#include <userver/components/component_base.hpp>

namespace write_service {

class AwsSdkComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "aws-sdk";

    AwsSdkComponent(const userver::components::ComponentConfig&,
                    const userver::components::ComponentContext&);
    ~AwsSdkComponent() override;
};

}