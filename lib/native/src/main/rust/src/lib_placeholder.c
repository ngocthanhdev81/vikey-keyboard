#include <jni.h>

JNIEXPORT jint JNICALL
Java_org_florisboard_libnative_TestKt_dummyAdd(JNIEnv *env, jclass clazz, jint a, jint b) {
    return a + b;
}
