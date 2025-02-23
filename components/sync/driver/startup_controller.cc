// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/startup_controller.h"

#include <utility>

#include "base/command_line.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_driver_switches.h"

namespace syncer {

namespace {

// The amount of time we'll wait to initialize sync if no data type requests
// immediately initialization.
const int kDefaultDeferredInitDelaySeconds = 10;

base::TimeDelta GetDeferredInitDelay() {
  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  if (cmdline->HasSwitch(switches::kSyncDeferredStartupTimeoutSeconds)) {
    int timeout = 0;
    if (base::StringToInt(cmdline->GetSwitchValueASCII(
                              switches::kSyncDeferredStartupTimeoutSeconds),
                          &timeout)) {
      DCHECK_GE(timeout, 0);
      DVLOG(2) << "Sync StartupController overriding startup timeout to "
               << timeout << " seconds.";
      return base::TimeDelta::FromSeconds(timeout);
    }
  }
  return base::TimeDelta::FromSeconds(kDefaultDeferredInitDelaySeconds);
}

bool IsDeferredStartupEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kSyncDisableDeferredStartup);
}

// Enum for UMA defining different events that cause us to exit the "deferred"
// state of initialization and invoke start_engine.
enum DeferredInitTrigger {
  // We have received a signal from a SyncableService requesting that sync
  // starts as soon as possible.
  TRIGGER_DATA_TYPE_REQUEST,
  // No data type requested sync to start and our fallback timer expired.
  TRIGGER_FALLBACK_TIMER,
  MAX_TRIGGER_VALUE
};

}  // namespace

StartupController::StartupController(
    const SyncPrefs* sync_prefs,
    base::RepeatingCallback<ModelTypeSet()> get_preferred_data_types,
    base::RepeatingCallback<bool()> can_start,
    base::RepeatingClosure start_engine)
    : sync_prefs_(sync_prefs),
      get_preferred_data_types_callback_(std::move(get_preferred_data_types)),
      can_start_callback_(std::move(can_start)),
      start_engine_callback_(std::move(start_engine)),
      bypass_setup_complete_(false),
      received_start_request_(false),
      setup_in_progress_(false),
      weak_factory_(this) {}

StartupController::~StartupController() {}

void StartupController::Reset() {
  received_start_request_ = false;
  bypass_setup_complete_ = false;
  start_up_time_ = base::Time();
  start_engine_time_ = base::Time();
  // Don't let previous timers affect us post-reset.
  weak_factory_.InvalidateWeakPtrs();
}

void StartupController::SetSetupInProgress(bool setup_in_progress) {
  setup_in_progress_ = setup_in_progress;
  if (setup_in_progress_) {
    TryStart();
  }
}

void StartupController::StartUp(StartUpDeferredOption deferred_option) {
  const bool first_start = start_up_time_.is_null();
  if (first_start) {
    start_up_time_ = base::Time::Now();
  }

  if (deferred_option == STARTUP_DEFERRED && IsDeferredStartupEnabled() &&
      get_preferred_data_types_callback_.Run().Has(SESSIONS)) {
    if (first_start) {
      base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&StartupController::OnFallbackStartupTimerExpired,
                     weak_factory_.GetWeakPtr()),
          GetDeferredInitDelay());
    }
    return;
  }

  if (start_engine_time_.is_null()) {
    start_engine_time_ = base::Time::Now();
    start_engine_callback_.Run();
  }
}

void StartupController::TryStart() {
  if (!can_start_callback_.Run()) {
    return;
  }

  // For performance reasons, defer the heavy lifting for sync init unless:
  //
  // - a datatype has requested an immediate start of sync, or
  // - sync needs to start up the engine immediately to provide control state
  //   and encryption information to the UI.
  // Do not start up the sync engine if setup has not completed and isn't
  // in progress, unless told to otherwise.
  if (setup_in_progress_) {
    StartUp(STARTUP_IMMEDIATE);
  } else if (sync_prefs_->IsFirstSetupComplete() || bypass_setup_complete_) {
    StartUp(received_start_request_ ? STARTUP_IMMEDIATE : STARTUP_DEFERRED);
  }
}

void StartupController::TryStartImmediately() {
  received_start_request_ = true;
  bypass_setup_complete_ = true;
  TryStart();
}

void StartupController::RecordTimeDeferred() {
  DCHECK(!start_up_time_.is_null());
  base::TimeDelta time_deferred = base::Time::Now() - start_up_time_;
  UMA_HISTOGRAM_CUSTOM_TIMES("Sync.Startup.TimeDeferred2", time_deferred,
                             base::TimeDelta::FromSeconds(0),
                             base::TimeDelta::FromMinutes(2), 60);
}

void StartupController::OnFallbackStartupTimerExpired() {
  DCHECK(IsDeferredStartupEnabled());

  if (!start_engine_time_.is_null())
    return;

  DVLOG(2) << "Sync deferred init fallback timer expired, starting engine.";
  RecordTimeDeferred();
  UMA_HISTOGRAM_ENUMERATION("Sync.Startup.DeferredInitTrigger",
                            TRIGGER_FALLBACK_TIMER, MAX_TRIGGER_VALUE);
  received_start_request_ = true;
  TryStart();
}

StartupController::State StartupController::GetState() const {
  if (!start_engine_time_.is_null())
    return State::STARTED;
  if (!start_up_time_.is_null())
    return State::STARTING_DEFERRED;
  return State::NOT_STARTED;
}

void StartupController::OnDataTypeRequestsSyncStartup(ModelType type) {
  if (!IsDeferredStartupEnabled()) {
    DVLOG(2) << "Ignoring data type request for sync startup: "
             << ModelTypeToString(type);
    return;
  }

  if (!start_engine_time_.is_null())
    return;

  DVLOG(2) << "Data type requesting sync startup: " << ModelTypeToString(type);
  // Measure the time spent waiting for init and the type that triggered it.
  // We could measure the time spent deferred on a per-datatype basis, but
  // for now this is probably sufficient.
  // TODO(wychen): enum uma should be strongly typed. crbug.com/661401
  UMA_HISTOGRAM_ENUMERATION("Sync.Startup.TypeTriggeringInit",
                            ModelTypeToHistogramInt(type),
                            static_cast<int>(MODEL_TYPE_COUNT));
  if (!start_up_time_.is_null()) {
    RecordTimeDeferred();
    UMA_HISTOGRAM_ENUMERATION("Sync.Startup.DeferredInitTrigger",
                              TRIGGER_DATA_TYPE_REQUEST, MAX_TRIGGER_VALUE);
  }
  received_start_request_ = true;
  TryStart();
}

}  // namespace syncer
