#include <string>
#include <set>
#include <map>
#include <vector>
#include "rail/sdk/rail_api.h"
#include <node.h>

using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Persistent;
using v8::Null;
using v8::Object;
using v8::String;
using v8::Boolean;
using v8::Number;
using v8::Array;
using v8::Value;

class CustomEvents : public rail::IRailEvent {
public:
	Persistent<Function> PlatformExitCallback;
	Persistent<Function> DlcStartDownloadCallback;
	Persistent<Function> DlcInstallProgressCallback;
	Persistent<Function> DlcInstallFinishCallback;

	CustomEvents();
	~CustomEvents();
	virtual void OnRailEvent(rail::RAIL_EVENT_ID event_id, rail::EventBase* param);

private:
	void RunPlatformExitCallback();
	void RunDlcStartDownloadCallback(bool isSucceed);
	void RunDlcInstallProgressCallback(bool isSucceed, uint32_t progress);
	void RunDlcInstallFinishCallback(bool isSucceed);
};
