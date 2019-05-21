#include <jni.h>

#include "com_broadcom_HlasmPlugin_HlasmPlugin.h"

#include "analyzer.h"
#include "jni_string_holder.h"

void write_debug(const char * message)
{
#ifdef _DEBUG
	std::cout << message;
#endif
}

JNIEXPORT jobjectArray JNICALL Java_com_broadcom_HlasmPlugin_HlasmPlugin_parseFile(JNIEnv * env, jobject this_obj, jstring file_name, jstring text)
{
	write_debug("Starting the parse\n");

	jni_string_holder file_name_js(env, file_name);
	jni_string_holder text_js(env, text);
	if (file_name_js.bad() || text_js.bad())
	{
		write_debug("Could not get the file_name and text\n");
		return nullptr;
	}

	std::string file_name_s(file_name_js.string(), (size_t)file_name_js.length());
	std::string text_s(text_js.string(), (size_t)text_js.length());

	write_debug("Got the strings\n");

	jclass diagnostic_class = env->FindClass("com/broadcom/HlasmPlugin/Diagnostic");
	if (diagnostic_class == nullptr)
	{
		write_debug("Could not find diagnostic class");
		return nullptr;
	}

	jmethodID diag_ctor_id = env->GetMethodID(diagnostic_class, "<init>", "(Ljava/lang/String;IIIIILjava/lang/String;Ljava/lang/String;)V");
	if (diag_ctor_id == nullptr)
	{
		write_debug("GetMethodID failed\n");
		return nullptr;
	}

	write_debug("Creating analyzer\n");
	hlasm_plugin::parser_library::analyzer analyzer(text_s, file_name_s);

	analyzer.analyze();

	analyzer.collect_diags();

	jobjectArray result = env->NewObjectArray(analyzer.diags().size(), diagnostic_class, nullptr);
	if (result == nullptr)
	{
		write_debug("NewObjectArray failed\n");
		return nullptr;
	}
	for (size_t i = 0; i < analyzer.diags().size(); ++i)
	{
		hlasm_plugin::parser_library::diagnostic_s & diag = analyzer.diags()[i];
		jstring file_name_str = env->NewStringUTF(diag.file_name.c_str());
		jint begin_ln = (jint)diag.diag_range.start.line;
		jint begin_col = (jint)diag.diag_range.start.column;
		jint end_ln = (jint)diag.diag_range.end.line;
		jint end_col = (jint)diag.diag_range.end.column;
		jint severity = (int)diag.severity;
		jstring message = env->NewStringUTF(diag.message.c_str());
		jstring source = env->NewStringUTF(diag.source.c_str());

		if (!file_name_str || !message || !source || env->ExceptionCheck())
		{
			write_debug("NewString failed\n");
			return nullptr;
		}

		jobject jdiag = env->NewObject(diagnostic_class, diag_ctor_id, file_name_str, begin_ln, begin_col, end_ln, end_col, severity, message, source);
		if (jdiag == nullptr || env->ExceptionCheck())
		{
			write_debug("Creating new diagnostic failed\n");
			return nullptr;
		}

		env->SetObjectArrayElement(result, i, jdiag);
		if(env->ExceptionCheck())
		{
			write_debug("SetObjectArrayElement failed\n");
			return nullptr;
		}
	}

	return result;
}
