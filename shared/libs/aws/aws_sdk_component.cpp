#include "aws/aws_sdk_component.hpp"

#include <aws/core/Aws.h>

static Aws::SDKOptions aws_options_;

namespace Aws {

AwsSdkComponent::AwsSdkComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context)
{
    Aws::InitAPI(aws_options_);
}

AwsSdkComponent::~AwsSdkComponent() {
    Aws::ShutdownAPI(aws_options_);
}

}