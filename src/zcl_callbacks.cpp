/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "bolt_lock_manager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/support/CodeUtils.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::DoorLock;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	VerifyOrReturn(attributePath.mClusterId == DoorLock::Id &&
		       attributePath.mAttributeId == DoorLock::Attributes::LockState::Id);

	/* Post events only if current lock state is different than given */
	switch (*value) {
	case to_underlying(DlLockState::kLocked):
		BoltLockMgr().Lock(BoltLockManager::OperationSource::kRemote);
		break;
	case to_underlying(DlLockState::kUnlocked):
		BoltLockMgr().Unlock(BoltLockManager::OperationSource::kRemote);
		break;
	default:
		break;
	}
}

bool emberAfPluginDoorLockOnDoorLockCommand(EndpointId endpointId, const Optional<ByteSpan> &pinCode,
					    OperationErrorEnum &err)
{
	BoltLockMgr().Lock(BoltLockManager::OperationSource::kRemote);
	return true;
}

bool emberAfPluginDoorLockOnDoorUnlockCommand(EndpointId endpointId, const Optional<ByteSpan> &pinCode,
					      OperationErrorEnum &err)
{
	/* Handle changing attribute state on command reception */
	BoltLockMgr().Unlock(BoltLockManager::OperationSource::kRemote);

	return true;
}

void emberAfDoorLockClusterInitCallback(EndpointId endpoint)
{
	DoorLockServer::Instance().InitServer(endpoint);

	const auto logOnFailure = [](EmberAfStatus status, const char *attributeName) {
		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			ChipLogError(Zcl, "Failed to set DoorLock %s: %x", attributeName, status);
		}
	};

	logOnFailure(DoorLock::Attributes::LockType::Set(endpoint, DlLockType::kDeadBolt), "type");

	AppTask::Instance().UpdateClusterState(BoltLockMgr().GetState(),
					       BoltLockManager::OperationSource::kUnspecified);
}
