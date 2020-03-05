#include "nfc-poll.h"
#include "tools.h"

using v8::Function;
using v8::Local;
using v8::Number;
using v8::Value;
using v8::Object;
using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::HandleScope;
using Nan::New;
using Nan::Null;
using Nan::To;
using Nan::Error;

using namespace std;

NFCPoll::NFCPoll(Nan::Callback *cb, nfc_device *device, nfc_modulation* modulations_data, const size_t& modulations_size, const uint8_t& uiPollNr, const uint8_t& uiPeriod)
:AsyncWorker(cb), _pnd(device), _modulations_size(modulations_size), _uiPollNr(uiPollNr), _uiPeriod(uiPeriod), _has_error(false) {
    memcpy(_modulations_data, modulations_data, modulations_size * sizeof(nfc_modulation));
}

void NFCPoll::Execute() {
    const uint8_t uiPollNr = _uiPollNr >= 0x01 && _uiPollNr <= 0xFF ? _uiPollNr : 20;
    const uint8_t uiPeriod = _uiPeriod >= 0x01 && _uiPeriod <= 0x0F ? _uiPeriod : 2;

    int res = 0;
    if ((res = nfc_initiator_poll_target(_pnd, _modulations_data, _modulations_size, uiPollNr, uiPeriod, &_nt))  <= 0) {
        _has_error = true;
        _error.assign(GetLibNFCError(res));
        return;
    }
}

const char* GetModulationType(const nfc_target &nt) {
    switch (nt.nm.nmt) {
        case NMT_ISO14443A:
            return "NMT_ISO14443A";
        case NMT_JEWEL:
            return "NMT_JEWEL";
        case NMT_ISO14443B:
            return "NMT_ISO14443B";
        case NMT_ISO14443BI:
            return "NMT_ISO14443BI";
        case NMT_ISO14443B2SR:
            return "NMT_ISO14443B2SR";
        case NMT_ISO14443B2CT:
            return "NMT_ISO14443B2CT";
        case NMT_FELICA:
            return "NMT_FELICA";
        case NMT_DEP:
            return "NMT_DEP";
        default:
            return "Unknown";
    }
}

const char *GetBaudRate(const nfc_target &nt) {
    switch (nt.nm.nbr) {
        case NBR_UNDEFINED:
            return "NBR_UNDEFINED";
        case NBR_106:
            return "NBR_106";
        case NBR_212:
            return "NBR_212";
        case NBR_424:
            return "NBR_424";
        case NBR_847:
            return "NBR_847";
    }
}

void NFCPoll::HandleOKCallback() {
    HandleScope scope;

    Local<Value> argv[] = {
        Null()
      , Null()
    };

    if(_has_error) {
        argv[0] = Error(_error.c_str());
    } else {
        Local<Object> obj = New<Object>();

        obj->Set(Nan::GetCurrentContext(), New("modulationType").ToLocalChecked(), New(GetModulationType(_nt)).ToLocalChecked());
        obj->Set(Nan::GetCurrentContext(), New("baudRate").ToLocalChecked(), New(GetBaudRate(_nt)).ToLocalChecked());
        argv[1] = obj;
    }

    callback->Call(2, argv);
}
