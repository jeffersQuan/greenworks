#include "CustomEvents.h"

extern HMODULE sdk_handle;

CustomEvents::CustomEvents() {
	if (sdk_handle != NULL) {
		rail::helper::Invoker invoker(sdk_handle);
		invoker.RailRegisterEvent(rail::kRailEventSystemStateChanged, this);
		invoker.RailRegisterEvent(rail::kRailEventDlcInstallStartResult, this);
		invoker.RailRegisterEvent(rail::kRailEventDlcInstallProgress, this);
		invoker.RailRegisterEvent(rail::kRailEventDlcInstallFinished, this);
	}
}

CustomEvents::~CustomEvents() {
	if (sdk_handle != NULL) {
		rail::helper::Invoker invoker(sdk_handle);
		invoker.RailUnregisterEvent(rail::kRailEventSystemStateChanged, this);
		invoker.RailUnregisterEvent(rail::kRailEventDlcInstallStartResult, this);
		invoker.RailUnregisterEvent(rail::kRailEventDlcInstallProgress, this);
		invoker.RailUnregisterEvent(rail::kRailEventDlcInstallFinished, this);
	}
}

void CustomEvents::OnRailEvent(rail::RAIL_EVENT_ID event_id, rail::EventBase* param) {
	if (event_id == rail::kRailEventSystemStateChanged) {
		rail::rail_event::RailSystemStateChanged* event =
			static_cast<rail::rail_event::RailSystemStateChanged*>(param);

		if (event->state == rail::kSystemStatePlatformOffline || event->state == rail::kSystemStatePlatformExit) {
			RunPlatformExitCallback();
		}
	} else if (event_id == rail::kRailEventDlcInstallStartResult) {
		rail::rail_event::DlcInstallStartResult* res = static_cast<rail::rail_event::DlcInstallStartResult*>(param);
		if (res->result == rail::kSuccess) {
			RunDlcStartDownloadCallback(true);
		} else {
			RunDlcStartDownloadCallback(false);
		}
	}
	else if (event_id == rail::kRailEventDlcInstallProgress) {
		rail::rail_event::DlcInstallProgress* res = static_cast<rail::rail_event::DlcInstallProgress*>(param);

		if (res->result == rail::kSuccess) {
			RunDlcInstallProgressCallback(true, res->progress.progress);
		} else {
			RunDlcInstallProgressCallback(false, 0);
		}
	}
	else if (event_id == rail::kRailEventDlcInstallFinished) {
		rail::rail_event::DlcInstallFinished* res = static_cast<rail::rail_event::DlcInstallFinished*>(param);

		if (res->result == rail::kSuccess) {
			RunDlcInstallFinishCallback(true);
		}
		else {
			RunDlcInstallFinishCallback(false);
		}
	}
}

void CustomEvents::RunPlatformExitCallback() {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, PlatformExitCallback);

	const unsigned argc = 1;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, true)
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
		PlatformExitCallback.Reset();
	}
}

void CustomEvents::RunDlcStartDownloadCallback(bool isSucceed) {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, DlcStartDownloadCallback);

	const unsigned argc = 1;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, isSucceed)
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
		DlcStartDownloadCallback.Reset();
	}
}

void CustomEvents::RunDlcInstallProgressCallback(bool isSucceed, uint32_t progress) {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, DlcInstallProgressCallback);
	double progressDouble = progress;

	const unsigned argc = 2;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, isSucceed),
		Number::New(isolate, progressDouble)
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
	}
}

void CustomEvents::RunDlcInstallFinishCallback(bool isSucceed) {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, DlcInstallFinishCallback);

	const unsigned argc = 1;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, isSucceed)
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
		DlcInstallFinishCallback.Reset();
		DlcInstallProgressCallback.Reset();
	}
}