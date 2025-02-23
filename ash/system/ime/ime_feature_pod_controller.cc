// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/ime/ime_feature_pod_controller.h"

#include "ash/ime/ime_controller.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/keyboard/keyboard_util.h"

namespace ash {

namespace {

bool IsButtonVisible() {
  DCHECK(Shell::Get());
  ImeController* ime_controller = Shell::Get()->ime_controller();
  size_t ime_count = ime_controller->available_imes().size();
  return ime_count > 1;
}

base::string16 GetLabelString() {
  DCHECK(Shell::Get());
  ImeController* ime_controller = Shell::Get()->ime_controller();
  size_t ime_count = ime_controller->available_imes().size();
  if (ime_count > 1) {
    return ime_controller->current_ime().short_name;
  } else {
    return l10n_util::GetStringUTF16(
        keyboard::IsKeyboardEnabled() ? IDS_ASH_STATUS_TRAY_KEYBOARD_ENABLED
                                      : IDS_ASH_STATUS_TRAY_KEYBOARD_DISABLED);
  }
}

}  // namespace

IMEFeaturePodController::IMEFeaturePodController(
    UnifiedSystemTrayController* tray_controller)
    : tray_controller_(tray_controller) {}

IMEFeaturePodController::~IMEFeaturePodController() = default;

FeaturePodButton* IMEFeaturePodController::CreateButton() {
  button_ = new FeaturePodButton(this);
  button_->SetVectorIcon(kSystemMenuKeyboardIcon);
  button_->SetLabel(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_IME_SHORT));
  Update();
  return button_;
}

void IMEFeaturePodController::OnIconPressed() {
  tray_controller_->ShowIMEDetailedView();
}

SystemTrayItemUmaType IMEFeaturePodController::GetUmaType() const {
  return SystemTrayItemUmaType::UMA_IME;
}

void IMEFeaturePodController::Update() {
  button_->SetSubLabel(GetLabelString());
  button_->SetVisible(IsButtonVisible());
}

}  // namespace ash
