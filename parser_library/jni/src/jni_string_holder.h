#include <jni.h>

class jni_string_holder
{
public:
	jni_string_holder(JNIEnv * env, jstring string) : env_(env), jstr_(string)
	{
		str_ = env->GetStringUTFChars(string, nullptr);
		if (str_ != nullptr)
		{
			length_ = env->GetStringUTFLength(string);
		}
	}

	const char * string()
	{
		return str_;
	}

	bool bad()
	{
		return str_ == nullptr;
	}

	jsize length()
	{
		return length_;
	}

	~jni_string_holder()
	{
		if (!bad())
			env_->ReleaseStringUTFChars(jstr_, str_);
	}

private:
	JNIEnv * env_;
	const char * str_;
	jstring jstr_;
	jsize length_;
};