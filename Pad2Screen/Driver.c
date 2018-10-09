#include "driver.h"
#include "driver.tmh"
#include <hidport.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, P2S_GetHidReportDescriptorInputMode)
#pragma alloc_text (PAGE, P2S_PatchHidReportDescriptor)
#pragma alloc_text (PAGE, P2S_IoctlHidSetFeatureCompletionRoutine)
#pragma alloc_text (PAGE, P2S_SetToPrecisionTouchpadMode)
#pragma alloc_text (PAGE, P2S_ForwardIoctlCompletionRoutine)
#pragma alloc_text (PAGE, P2S_EvtIoDeviceControl)
#pragma alloc_text (PAGE, P2S_EvtDriverContextCleanup)
#pragma alloc_text (PAGE, P2S_EvtDeviceAdd)
#pragma alloc_text (INIT, DriverEntry)
#endif

// HID report descriptor constants
#define HID_TYPE_COLLECTION 0xA1
#define HID_TYPE_END_COLLECTION 0xC0
#define HID_TYPE_USAGE_PAGE 0x05
#define HID_TYPE_USAGE 0x09
#define HID_TYPE_REPORT_ID 0x85
#define HID_TYPE_REPORT_SIZE 0x75
#define HID_TYPE_REPORT_COUNT 0x95
#define HID_TYPE_FEATURE 0xB1
#define HID_USAGE_PAGE_DIGITIZERS 0x0D
#define HID_USAGE_TOUCHPAD 0x05
#define HID_USAGE_TOUCHSCREEN 0x04
#define HID_USAGE_CONFIGURATION 0x0E
#define HID_USAGE_INPUT_MODE 0x52

bool
P2S_GetHidReportDescriptorInputMode(
	_In_ byte *descriptor,
	_In_ size_t descriptorLen,
	_Out_ byte *inputModeReportId,
	_Out_ size_t *inputModeReportSize)
{
	int depth = 0;
	byte usagePage = 0;
	byte reportId = 0;
	byte reportSize = 0;
	byte reportCount = 0;
	byte lastUsage = 0;
	bool inConfigTlc = false;

	for (size_t i = 0; i < descriptorLen;) {
		byte type = descriptor[i++];
		int size = type & 3;
		if (size == 3) {
			size++;
		}
		byte *value = &descriptor[i];
		i += size;

		// WARNING: The following code makes a ton of assumptions
		// to avoid having to parse the HID report descriptor.
		// It assumes that there will only be a single field in
		// the "input mode" usage. This is b/c Microsoft specifies
		// the field should contain either 0 or 3, and there aren't
		// many ways to achieve that. It also doesn't support
		// push/pop/pretty much any other aspect of HID.
		if (type == HID_TYPE_COLLECTION) {
			depth++;
			if (depth == 1 && usagePage == HID_USAGE_PAGE_DIGITIZERS && lastUsage == HID_USAGE_CONFIGURATION) {
				inConfigTlc = true;
			}
		} else if (type == HID_TYPE_END_COLLECTION) {
			depth--;
		} else if (type == HID_TYPE_USAGE_PAGE) {
			usagePage = *value;
		} else if (type == HID_TYPE_USAGE) {
			lastUsage = *value;
		} else if (type == HID_TYPE_REPORT_ID) {
			reportId = *value;
		} else if (type == HID_TYPE_REPORT_SIZE) {
			reportSize = *value;
		} else if (type == HID_TYPE_REPORT_COUNT) {
			reportCount = *value;
		} else if (inConfigTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_INPUT_MODE) {
			*inputModeReportSize = (reportSize + 7) / 8;
			*inputModeReportId = reportId;
			return true;
		}
	}

	return false;
}

bool
P2S_PatchHidReportDescriptor(
	_Inout_ byte *descriptor,
	_In_ size_t descriptorLen)
{
	// Warning: the following fields are required in touchscreen drivers
	// but are not required in touchpad drivers. Currently the "required"
	// fields are actually optional, but that might change in the future.
	//
	// In-range (page 0x0D, usage 0x32)
	// Contact count maximum (page 0x0D, usage 0x55)
	//
	// Touchscreen spec:
	// https://docs.microsoft.com/en-us/windows-hardware/design/component-guidelines/supporting-usages-in-multitouch-digitizer-drivers
	//
	// Touchpad spec:
	// https://docs.microsoft.com/en-us/windows-hardware/design/component-guidelines/windows-precision-touchpad-required-hid-top-level-collections
	
	// Below is a really dumb HID "parser" that only recognizes page/usage
	// and skips everything else.
	int depth = 0;
	byte usagePage = 0;
	bool patched = false;
	for (size_t i = 0; i < descriptorLen;) {
		byte type = descriptor[i++];
		int size = type & 3;
		if (size == 3) {
			size++;
		}
		byte *value = &descriptor[i];
		i += size;

		if (type == HID_TYPE_COLLECTION) {
			depth++;
		} else if (type == HID_TYPE_END_COLLECTION) {
			depth--;
		} else if (type == HID_TYPE_USAGE_PAGE) {
			usagePage = *value;
		} else if (depth == 0 && type == HID_TYPE_USAGE) {
			if (usagePage == HID_USAGE_PAGE_DIGITIZERS && *value == HID_USAGE_TOUCHPAD) {
				*value = HID_USAGE_TOUCHSCREEN;
				patched = true;
			}
		}
	}

	return patched;
}

const char *
P2S_IoctlCodeToString(
	_In_ unsigned long ioControlCode)
{
	switch (ioControlCode) {
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
	case IOCTL_HID_READ_REPORT:
		return "IOCTL_HID_READ_REPORT";
	case IOCTL_HID_WRITE_REPORT:
		return "IOCTL_HID_WRITE_REPORT";
	case IOCTL_HID_GET_FEATURE:
		return "IOCTL_HID_GET_FEATURE";
	case IOCTL_HID_SET_FEATURE:
		return "IOCTL_HID_SET_FEATURE";
	case IOCTL_HID_GET_INPUT_REPORT:
		return "IOCTL_HID_GET_INPUT_REPORT";
	case IOCTL_HID_SET_OUTPUT_REPORT:
		return "IOCTL_HID_SET_OUTPUT_REPORT";
	case IOCTL_UMDF_HID_GET_FEATURE:
		return "IOCTL_UMDF_HID_GET_FEATURE";
	case IOCTL_UMDF_HID_SET_FEATURE:
		return "IOCTL_UMDF_HID_SET_FEATURE";
	case IOCTL_UMDF_HID_GET_INPUT_REPORT:
		return "IOCTL_UMDF_HID_GET_INPUT_REPORT";
	case IOCTL_UMDF_HID_SET_OUTPUT_REPORT:
		return "IOCTL_UMDF_HID_SET_OUTPUT_REPORT";
	case IOCTL_HID_GET_STRING:
		return "IOCTL_HID_GET_STRING";
	case IOCTL_HID_GET_INDEXED_STRING:
		return "IOCTL_HID_GET_INDEXED_STRING";
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
	case IOCTL_HID_ACTIVATE_DEVICE:
		return "IOCTL_HID_ACTIVATE_DEVICE";
	case IOCTL_HID_DEACTIVATE_DEVICE:
		return "IOCTL_HID_DEACTIVATE_DEVICE";
	case IOCTL_GET_PHYSICAL_DESCRIPTOR:
		return "IOCTL_GET_PHYSICAL_DESCRIPTOR";
	default:
		return "<unknown ioctl>";
	}
}

void
P2S_IoctlHidSetFeatureCompletionRoutine(
	_In_ WDFREQUEST request,
	_In_ WDFIOTARGET target,
	_In_ PWDF_REQUEST_COMPLETION_PARAMS completionParams,
	_In_ WDFCONTEXT context)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(target);
	UNREFERENCED_PARAMETER(completionParams);
	UNREFERENCED_PARAMETER(context);
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!()");

	// Log any errors that occurred
	status = WdfRequestGetStatus(request);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ioctl(IOCTL_HID_SET_FEATURE) failed: %!STATUS!", status);
	}

	// Destroy the request since we're done with it now
	WdfObjectDelete(request);
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!() -> %!STATUS!", status);
}

NTSTATUS
P2S_SetToPrecisionTouchpadMode(
	_In_ WDFIOTARGET target,
	_In_ byte *descriptor,
	_In_ size_t descriptorLen)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFREQUEST request = NULL;
	WDFMEMORY inputMemory = NULL;
	byte reportId;
	size_t reportSize;
	void *inputBuffer;
	byte *report;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!()");

	// As per the precision touchpad spec, touchpads shall start
	// in mouse emulation mode and be set to precision mode via
	// a HID feature report. Since Windows thinks we're a touchscreen,
	// we should manually send this report to the touchpad driver.

	// Determine report ID and the size of the feature field
	if (!P2S_GetHidReportDescriptorInputMode(descriptor, descriptorLen, &reportId, &reportSize)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "P2S_GetHidReportDescriptorInputMode() did not find input mode");
		goto exit;
	}

	// Create ioctl request
	status = WdfRequestCreate(NULL, target, &request);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestCreate() failed: %!STATUS!", status);
		goto exit;
	}

	// Allocate buffer for request
	status = WdfMemoryCreate(NULL, PagedPool, 0, reportSize + 1, &inputMemory, &inputBuffer);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate() failed: %!STATUS!", status);
		goto exit;
	}

	// Fill out the buffer. First byte is the report ID, remaining bytes
	// combine to form the request. We assumed that the report contains
	// only a single field (the input mode), and that it's the first field
	// before any padding.
	report = inputBuffer;
	report[0] = reportId;
	report[1] = 3;
	for (size_t i = 0; i < reportSize; ++i) {
		report[i] = 0;
	}

	// Assign buffer to request
	status = WdfIoTargetFormatRequestForIoctl(target, request, IOCTL_HID_SET_FEATURE, inputMemory, NULL, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfIoTargetFormatRequestForIoctl() failed: %!STATUS!", status);
		goto exit;
	}
	
	// Memory is owned by the request now
	inputMemory = NULL;

	// Asynchronously send the ioctl request
	WdfRequestSetCompletionRoutine(request, P2S_IoctlHidSetFeatureCompletionRoutine, NULL);
	if (!WdfRequestSend(request, target, NULL)) {
		status = WdfRequestGetStatus(request);
		goto exit;
	}

	// Request will be cleaned up in the completion routine
	request = NULL;

exit:
	if (request != NULL) {
		WdfObjectDelete(request);
	}
	if (inputMemory != NULL) {
		WdfObjectDelete(inputMemory);
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!() -> %!STATUS!", status);
	return status;
}

void
P2S_ForwardIoctlCompletionRoutine(
	_In_ WDFREQUEST request,
	_In_ WDFIOTARGET target,
	_In_ PWDF_REQUEST_COMPLETION_PARAMS completionParams,
	_In_ WDFCONTEXT context)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFMEMORY outputBuffer = NULL;
	byte *buf;
	byte *descriptor;
	size_t descriptorLen;

	UNREFERENCED_PARAMETER(target);
	UNREFERENCED_PARAMETER(context);
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!(%s)", P2S_IoctlCodeToString(completionParams->Parameters.Ioctl.IoControlCode));

	// If the real ioctl failed, don't try to patch
	status = completionParams->IoStatus.Status;
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Forwarded ioctl() failed: %!STATUS!", status);
		goto exit;
	}

	// Only patch requests for the HID report descriptor
	if (completionParams->Parameters.Ioctl.IoControlCode != IOCTL_HID_GET_REPORT_DESCRIPTOR) {
		goto exit;
	}

	// Get the buffer that the report descriptor
	// was copied to
	outputBuffer = completionParams->Parameters.Ioctl.Output.Buffer;
	buf = WdfMemoryGetBuffer(outputBuffer, NULL);

	// Descriptor starts at buf offset (or at least it should)
	// Information holds the length of the descriptor
	descriptor = buf + completionParams->Parameters.Ioctl.Output.Offset;
	descriptorLen = completionParams->IoStatus.Information;
	if (P2S_PatchHidReportDescriptor(descriptor, descriptorLen)) {
		// If the device is indeed a precision touchpad, change it
		// from mouse emulation mode to precision touchpad mode.
		// Ignore errors and pray that the device works even if the
		// ioctl fails.
		P2S_SetToPrecisionTouchpadMode(target, descriptor, descriptorLen);
	} else {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "P2S_PatchHidReportDescriptor() did not find touchpad usage");
	}

exit:
	// Pass the results up to our caller
	WdfRequestCompleteWithInformation(request, status, completionParams->IoStatus.Information);
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!() -> %!STATUS!", status);
}

void
P2S_EvtIoDeviceControl(
	_In_ WDFQUEUE queue,
	_In_ WDFREQUEST request,
	_In_ size_t outputBufferLength,
	_In_ size_t inputBufferLength,
	_In_ unsigned long ioControlCode)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE device = NULL;
	WDFIOTARGET target = NULL;
	WDFMEMORY inputMemory = NULL;
	WDFMEMORY outputMemory = NULL;

	UNREFERENCED_PARAMETER(outputBufferLength);
	UNREFERENCED_PARAMETER(inputBufferLength);
	UNREFERENCED_PARAMETER(ioControlCode);
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!(%s)", P2S_IoctlCodeToString(ioControlCode));

	// Get a reference to the real driver that will
	// be handling the ioctl request.
	device = WdfIoQueueGetDevice(queue);
	target = WdfDeviceGetIoTarget(device);
	if (target == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceGetIoTarget() == NULL");
		status = STATUS_NOT_SUPPORTED;
		goto exit;
	}

	// Format request. Apparently WdfRequestFormatRequestUsingCurrentType()
	// is a big fat phony and doesn't actually work here. Following code is
	// taken from the kbfiltr sample driver.
	status = WdfRequestRetrieveInputMemory(request, &inputMemory);
	if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveInputMemory() failed: %!STATUS!", status);
		goto exit;
	}
	status = WdfRequestRetrieveOutputMemory(request, &outputMemory);
	if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputMemory() failed: %!STATUS!", status);
		goto exit;
	}
	status = WdfIoTargetFormatRequestForInternalIoctl(target, request, ioControlCode, inputMemory, NULL, outputMemory, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfIoTargetFormatRequestForInternalIoctl() failed: %!STATUS!", status);
		goto exit;
	}

	// Forward the request to the real driver
	WdfRequestSetCompletionRoutine(request, P2S_ForwardIoctlCompletionRoutine, NULL);
	if (!WdfRequestSend(request, target, NULL)) {
		status = WdfRequestGetStatus(request);
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestSend() failed: %!STATUS!", status);
		goto exit;
	}

	status = STATUS_SUCCESS;

exit:
	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(request, status);
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!() -> %!STATUS!", status);
}

NTSTATUS
P2S_EvtDeviceAdd(
	_In_ WDFDRIVER driver,
	_Inout_ PWDFDEVICE_INIT deviceInit)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE device = NULL;
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_IO_QUEUE_CONFIG queueConfig;

	UNREFERENCED_PARAMETER(driver);
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!()");

	// Set filter mode to get all non-ioctl requests to be
	// automatically forwarded to the real device driver
	WdfFdoInitSetFilter(deviceInit);

	// Create device object
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
	status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &device);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceCreate() failed: %!STATUS!", status);
		goto exit;
	}

	// Create I/O queue in parallel mode since we don't
	// have any global state to worry about
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
	queueConfig.EvtIoInternalDeviceControl = P2S_EvtIoDeviceControl;
	status = WdfIoQueueCreate(device, &queueConfig, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfIoQueueCreate() failed: %!STATUS!", status);
		goto exit;
	}

exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!() -> %!STATUS!", status);
	return status;
}

void
P2S_EvtDriverContextCleanup(
	_In_ WDFOBJECT driverObject)
{
	PAGED_CODE();

	// stop tracing
	WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)driverObject));
}

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT driverObject,
	_In_ PUNICODE_STRING registryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_DRIVER_CONFIG config;

	// Initialize tracing
	WPP_INIT_TRACING(driverObject, registryPath);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Enter %!FUNC!()");

	// Set up callbacks
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = P2S_EvtDriverContextCleanup;
	WDF_DRIVER_CONFIG_INIT(&config, P2S_EvtDeviceAdd);

	// Create driver object
	status = WdfDriverCreate(driverObject, registryPath, &attributes, &config, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate() failed: %!STATUS!", status);
		goto exit;
	}

exit:
	// Stop tracing if driver failed to init
	if (!NT_SUCCESS(status)) {
		WPP_CLEANUP(driverObject);
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Exit %!FUNC!()");
	return status;
}
