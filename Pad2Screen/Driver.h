#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "trace.h"

EXTERN_C_START

typedef struct _DEVICE_CONTEXT {
	ULONG PrivateDeviceData;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

typedef unsigned char byte;
typedef enum { false, true } bool;

typedef bool
P2S_GET_HID_REPORT_DESCRIPTOR_INPUT_MODE(
	_In_ byte *descriptor,
	_In_ size_t descriptorLen,
	_Out_ byte *inputModeReportId,
	_Out_ size_t *inputModeReportSize);

typedef bool
P2S_PATCH_HID_REPORT_DESCRIPTOR(
	_Inout_ byte *descriptor,
	_In_ size_t descriptorLen);

typedef NTSTATUS
P2S_SET_TO_PRECISION_TOUCHPAD_MODE(
	_In_ WDFIOTARGET target,
	_In_ byte *descriptor,
	_In_ size_t descriptorLen);

P2S_GET_HID_REPORT_DESCRIPTOR_INPUT_MODE P2S_GetHidReportDescriptorInputMode;
P2S_PATCH_HID_REPORT_DESCRIPTOR P2S_PatchHidReportDescriptor;
EVT_WDF_REQUEST_COMPLETION_ROUTINE P2S_IoctlHidSetFeatureCompletionRoutine;
P2S_SET_TO_PRECISION_TOUCHPAD_MODE P2S_SetToPrecisionTouchpadMode;
EVT_WDF_REQUEST_COMPLETION_ROUTINE P2S_ForwardIoctlCompletionRoutine;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL P2S_EvtIoDeviceControl;
EVT_WDF_OBJECT_CONTEXT_CLEANUP P2S_EvtDriverContextCleanup;
EVT_WDF_DRIVER_DEVICE_ADD P2S_EvtDeviceAdd;
DRIVER_INITIALIZE DriverEntry;

EXTERN_C_END
