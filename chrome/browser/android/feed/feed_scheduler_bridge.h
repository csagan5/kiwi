// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_FEED_FEED_SCHEDULER_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_FEED_FEED_SCHEDULER_BRIDGE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace feed {

class FeedSchedulerHost;

// Native counterpart of FeedSchedulerBridge.java. Holds non-owning pointers to
// native implementation, to which operations are delegated. Also capable of
// calling back into Java half.
class FeedSchedulerBridge {
 public:
  FeedSchedulerBridge(const base::android::JavaRef<jobject>& j_this,
                      FeedSchedulerHost* scheduler_host);
  ~FeedSchedulerBridge();

  void Destroy(JNIEnv* env, const base::android::JavaRef<jobject>& j_this);

  jint ShouldSessionRequestData(JNIEnv* env,
                                const base::android::JavaRef<jobject>& j_this,
                                const jboolean j_has_content,
                                const jlong j_content_creation_date_time_ms,
                                const jboolean j_has_outstanding_request);

  void OnReceiveNewContent(JNIEnv* env,
                           const base::android::JavaRef<jobject>& j_this,
                           const jlong j_content_creation_date_time_ms);

  void OnRequestError(JNIEnv* env,
                      const base::android::JavaRef<jobject>& j_this,
                      jint j_network_response_code);

  void OnForegrounded(JNIEnv* env,
                      const base::android::JavaRef<jobject>& j_this);

  void OnFixedTimer(JNIEnv* env, const base::android::JavaRef<jobject>& j_this);

  // Callable by native code to invoke Java code. Sends a request to the Feed
  // library to make the refresh call.
  void TriggerRefresh();

 private:
  // Reference to the Java half of this bridge. Always valid.
  base::android::ScopedJavaGlobalRef<jobject> j_this_;

  // Object to which all Java to native calls are delegated.
  FeedSchedulerHost* scheduler_host_;

  base::WeakPtrFactory<FeedSchedulerBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FeedSchedulerBridge);
};

}  // namespace feed

#endif  // CHROME_BROWSER_ANDROID_FEED_FEED_SCHEDULER_BRIDGE_H_
