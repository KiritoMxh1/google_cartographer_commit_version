/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cartographer/common/make_unique.h"

#include "cartographer_ros/msg_conversion.h"
#include "cartographer_ros/tf_bridge.h"

namespace cartographer_ros {

TfBridge::TfBridge(const std::string& tracking_frame,
                   const double lookup_transform_timeout_sec,
                   const tf2_ros::Buffer* buffer)
    : tracking_frame_(tracking_frame),
      lookup_transform_timeout_sec_(lookup_transform_timeout_sec),
      buffer_(buffer) {}

// 查找 从给定"frame_id"坐标系到'tracking_frame_'坐标系的变换, 并返回Rigid3d类型的变换
std::unique_ptr<::cartographer::transform::Rigid3d> TfBridge::LookupToTracking(
        const ::cartographer::common::Time time,                    //时间戳
        const std::string& frame_id) const {                        //给定"frame_id"坐标系

    // 设置超时时间间隔
    ::ros::Duration timeout(lookup_transform_timeout_sec_);
    // 这是要输出的东西
    std::unique_ptr<::cartographer::transform::Rigid3d> frame_id_to_tracking;
    try {
        // 查找 从给定"frame_id"坐标系到'tracking_frame_'坐标系的变换, 如果找到, 返回对应tf变换的时间戳
        const ::ros::Time latest_tf_time =
                buffer_
                ->lookupTransform(tracking_frame_, frame_id, ::ros::Time(0.),
                                  timeout)
                .header.stamp;
        // 时间检查, 如果最新的tf变换时间戳比请求的时间还晚
        const ::ros::Time requested_time = ToRos(time);
        if (latest_tf_time >= requested_time) {
            // We already have newer data, so we do not wait. Otherwise, we would wait
            // for the full 'timeout' even if we ask for data that is too old.
            // 表示已经有更新的数据,所以不再等待
            timeout = ::ros::Duration(0.);
        }
        // 返回Rigid3d类型的 从给定"frame_id"坐标系到'tracking_frame_'坐标系的变换
        return ::cartographer::common::make_unique<
                ::cartographer::transform::Rigid3d>(ToRigid3d(buffer_->lookupTransform(
                                                                  tracking_frame_, frame_id, requested_time, timeout)));
    } catch (const tf2::TransformException& ex) {
        LOG(WARNING) << ex.what();
    }
    return nullptr;
}

}  // namespace cartographer_ros
