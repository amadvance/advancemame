#include "ntddk.h"

#include "svgacode.h"

#if DBG
#define svgalibKdPrint(arg) DbgPrint arg
#else
#define svgalibKdPrint(arg)
#endif

NTSTATUS svgalib_map(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_MAP_IN* in = (SVGALIB_MAP_IN*)IoBuffer;
	SVGALIB_MAP_OUT* out = (SVGALIB_MAP_OUT*)IoBuffer;
	ULONG busNumber;
	PHYSICAL_ADDRESS physicalAddress;
	ULONG length;
	UNICODE_STRING physicalMemoryUnicodeString;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE physicalMemoryHandle = NULL;
	PVOID PhysicalMemorySection = NULL;
	ULONG inIoSpace1, inIoSpace2;
	NTSTATUS status;
	PHYSICAL_ADDRESS physicalAddressBase;
	PHYSICAL_ADDRESS physicalAddressEnd;
	PHYSICAL_ADDRESS viewBase;
	PHYSICAL_ADDRESS mappedLength;
	BOOLEAN translateBaseAddress;
	BOOLEAN translateEndAddress;
	PVOID virtualAddress;

	if (InputBufferLength != sizeof(SVGALIB_MAP_IN) || OutputBufferLength != sizeof(SVGALIB_MAP_OUT)) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	busNumber = in->bus;
	physicalAddress = in->address;
	inIoSpace1 = 0;
	inIoSpace2 = 0;
	length = in->size;

	// Get a pointer to physical memory...
	// - Create the name
	// - Initialize the data to find the object
	// - Open a handle to the oject and check the status
	// - Get a pointer to the object
	// - Free the handle
	RtlInitUnicodeString(&physicalMemoryUnicodeString, L"\\Device\\PhysicalMemory");

	InitializeObjectAttributes(&objectAttributes, &physicalMemoryUnicodeString, OBJ_CASE_INSENSITIVE, (HANDLE) NULL, (PSECURITY_DESCRIPTOR) NULL);

	status = ZwOpenSection(&physicalMemoryHandle, SECTION_ALL_ACCESS, &objectAttributes);
	if (!NT_SUCCESS(status)) {
		svgalibKdPrint(("SVGAWIN: ZwOpenSection failed\n"));
		goto err;
	}

	status = ObReferenceObjectByHandle(physicalMemoryHandle, SECTION_ALL_ACCESS, (POBJECT_TYPE) NULL, KernelMode, &PhysicalMemorySection, (POBJECT_HANDLE_INFORMATION) NULL);

	if (!NT_SUCCESS(status)){
		svgalibKdPrint(("SVGAWIN: ObReferenceObjectByHandle failed\n"));
		goto err_handle;
	}

	// Initialize the physical addresses that will be translated
	physicalAddressEnd = RtlLargeIntegerAdd(physicalAddress, RtlConvertUlongToLargeInteger(length));

	// Translate the physical addresses.
	translateBaseAddress = HalTranslateBusAddress(PCIBus, busNumber, physicalAddress, &inIoSpace1, &physicalAddressBase);
	translateEndAddress = HalTranslateBusAddress(PCIBus, busNumber, physicalAddressEnd, &inIoSpace2, &physicalAddressEnd);

	if (!(translateBaseAddress && translateEndAddress)) {
		svgalibKdPrint(("SVGAWIN: HalTranslatephysicalAddress failed\n"));
		status = STATUS_UNSUCCESSFUL;
		goto err_handle;
	}

	// Calculate the length of the memory to be mapped
	mappedLength = RtlLargeIntegerSubtract(physicalAddressEnd, physicalAddressBase);

	// If the mappedlength is zero, something very weird happened in the HAL
	// since the Length was checked against zero.
	if (mappedLength.LowPart == 0) {
		svgalibKdPrint(("SVGAWIN: mappedLength.LowPart == 0\n"));
		status = STATUS_UNSUCCESSFUL;
		goto err_handle;
	}

	length = mappedLength.LowPart;

	// initialize view base that will receive the physical mapped
	// address after the MapViewOfSection call.
	viewBase = physicalAddressBase;

	// Let ZwMapViewOfSection pick an address
	virtualAddress = NULL;

	// Map the section
	status = ZwMapViewOfSection(physicalMemoryHandle, (HANDLE)-1, &virtualAddress, 0L, length, &viewBase, &length, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);

	if (!NT_SUCCESS(status)) {
		svgalibKdPrint(("SVGAWIN: ZwMapViewOfSection failed\n"));
		goto err_handle;
	}

	// Mapping the section above rounded the physical address down to the
	// nearest 64 K boundary. Now return a virtual address that sits where
	// we want by adding in the offset from the beginning of the section.
	(ULONG) virtualAddress += (ULONG)physicalAddressBase.LowPart - (ULONG)viewBase.LowPart;

	out->address = virtualAddress;

	ZwClose(physicalMemoryHandle);
	
	status = STATUS_SUCCESS;
	return status;

err_handle:
	ZwClose(physicalMemoryHandle);
err:
	return status;
}

NTSTATUS svgalib_unmap(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_UNMAP_IN* in = (SVGALIB_UNMAP_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_UNMAP_IN) || OutputBufferLength != 0) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	status = ZwUnmapViewOfSection((HANDLE)-1, in->address);
	if (!NT_SUCCESS(status)) {
		svgalibKdPrint(("SVGAWIN: ZwUnmapViewOfSection failed\n"));
		goto err;
	}

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

unsigned char inportb(unsigned _port) {
	unsigned char rv;
	__asm mov edx,[_port]
	__asm in al, dx
	__asm mov [rv], al
	return rv;
}

unsigned short inportw(unsigned _port) {
	unsigned short rv;
	__asm mov edx,[_port]
	__asm in ax, dx
	__asm mov [rv], ax
	return rv;
}

unsigned inportl(unsigned _port) {
	unsigned rv;
	__asm mov edx,[_port]
	__asm in eax, dx
	__asm mov [rv], eax  
	return rv;
}

void outportb(unsigned _port, unsigned char _data) {
	__asm mov edx, [_port]
	__asm mov al, [_data]
	__asm out dx, al
}

void outportw(unsigned _port, unsigned short _data) {
	__asm mov edx, [_port]
	__asm mov ax, [_data]
	__asm out dx, ax
}

void outportl(unsigned _port, unsigned _data) {
	__asm mov edx, [_port]
	__asm mov eax, [_data]
	__asm out dx, eax
}

NTSTATUS svgalib_port_read(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PORT_READ_IN* in = (SVGALIB_PORT_READ_IN*)IoBuffer;
	SVGALIB_PORT_READ_OUT* out = (SVGALIB_PORT_READ_OUT*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PORT_READ_IN) || OutputBufferLength != sizeof(SVGALIB_PORT_READ_OUT)) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	switch (in->size) {
	case 1 :
		out->data = inportb(in->port);
		break;
	case 2 :
		out->data = inportw(in->port);
		break;
	case 4 :
		out->data = inportl(in->port);
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

NTSTATUS svgalib_port_write(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PORT_WRITE_IN* in = (SVGALIB_PORT_WRITE_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PORT_WRITE_IN) || OutputBufferLength != 0) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	switch (in->size) {
	case 1 :
		outportb(in->port, (UCHAR)in->data);
		break;
	case 2 :
		outportw(in->port, (USHORT)in->data);
		break;
	case 4 :
		outportl(in->port, (ULONG)in->data);
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

NTSTATUS svgalib_pci_bus(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_BUS_OUT* out = (SVGALIB_PCI_BUS_OUT*)IoBuffer;
	NTSTATUS status;
	ULONG bus;
	UCHAR data[4];

	if (InputBufferLength != 0 || OutputBufferLength != sizeof(SVGALIB_PCI_BUS_OUT)) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	bus = 0;
	while (bus < 255 && HalGetBusData(PCIConfiguration, bus, 0, &data, 4) != 0) {
		++bus;
	}

	out->bus = bus;

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

NTSTATUS svgalib_pci_read(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_READ_IN* in = (SVGALIB_PCI_READ_IN*)IoBuffer;
	SVGALIB_PCI_READ_OUT* out = (SVGALIB_PCI_READ_OUT*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PCI_READ_IN) || OutputBufferLength != sizeof(SVGALIB_PCI_READ_OUT)) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	if (HalGetBusDataByOffset(
		PCIConfiguration,
		in->bus_device_func >> 8,
		in->bus_device_func & 0xFF,
		&out->data,
		in->offset,
		in->size
	) != in->size) {
		status = STATUS_UNSUCCESSFUL;
		goto err;
	} 

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

NTSTATUS svgalib_pci_write(IN PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_WRITE_IN* in = (SVGALIB_PCI_WRITE_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PCI_WRITE_IN) || OutputBufferLength != 0) {
		svgalibKdPrint(("SVGAWIN: Invalid input or output buffer\n"));
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	if (HalSetBusDataByOffset(
		PCIConfiguration,
		in->bus_device_func >> 8,
		in->bus_device_func & 0xFF,
		&in->data,
		in->offset,
		in->size
	) != in->size) {
		status = STATUS_UNSUCCESSFUL;
		goto err;
	} 

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

/**
 * This is the "structure" of the IOPM.  It is just a simple
 * character array of length 0x2000.
 *
 * This holds 8K * 8 bits -> 64K bits of the IOPM, which maps the
 * entire 64K I/O space of the x86 processor.  Any 0 bits will give
 * access to the corresponding port for user mode processes.  Any 1
 * bits will disallow I/O access to the corresponding port.
 */
#define IOPM_SIZE 0x2000
typedef UCHAR IOPM[IOPM_SIZE];

/**
 * This will hold simply an array of 0's which will be copied
 * into our actual IOPM in the TSS by Ke386SetIoAccessMap().
 * The memory is allocated at driver load time.
 */
IOPM* IOPM_local = 0;

/**
 * These are the two undocumented calls that we will use to give
 * the calling process I/O access.
 *
 * Ke386IoSetAccessMap() copies the passed map to the TSS.
 *
 * Ke386IoSetAccessProcess() adjusts the IOPM offset pointer so that
 * the newly copied map is actually used.  Otherwise, the IOPM offset
 * points beyond the end of the TSS segment limit, causing any I/O
 * access by the user mode process to generate an exception.
 */
void Ke386SetIoAccessMap(int, IOPM*);
void Ke386QueryIoAccessMap(int, IOPM*);
void Ke386IoSetAccessProcess(PEPROCESS, int);

/***
 * Set the IOPM (I/O permission map) of the calling process so that it
 * is given full I/O access.  Our IOPM_local[] array is all zeros, so
 * the IOPM will be all zeros.
 */
VOID svgalib_giveio_on()
{ 
	Ke386IoSetAccessProcess(PsGetCurrentProcess(), 1);
	Ke386SetIoAccessMap(1, IOPM_local);
}

VOID svgalib_giveio_off()
{ 
	Ke386IoSetAccessProcess(PsGetCurrentProcess(), 0);
	Ke386SetIoAccessMap(1, IOPM_local);
}

/**
 * Make sure our structure is packed properly, on byte boundary, not
 * on the default doubleword boundary.
 */
#pragma pack(push,1)

/**
 * Structures for manipulating the GDT register and a GDT segment
 * descriptor entry.  Documented in Intel processor handbooks.
 */
typedef struct {
	unsigned limit : 16;
	unsigned baselo : 16;
	unsigned basemid : 8;
	unsigned type : 4;
	unsigned system : 1;
	unsigned dpl : 2;
	unsigned present : 1;
	unsigned limithi : 4;
	unsigned available : 1;
	unsigned zero : 1;
	unsigned size : 1;
	unsigned granularity : 1;
	unsigned basehi : 8;
} GDTENT;

typedef struct {
	unsigned short limit;
	GDTENT* base;
} GDTREG;

#pragma pack(pop)

/**
 * This is the lowest level for setting the TSS segment descriptor
 * limit field.  We get the selector ID from the STR instruction,
 * index into the GDT, and poke in the new limit.  In order for the
 * new limit to take effect, we must then read the task segment
 * selector back into the task register (TR).
 */
void SetTSSLimit(int size)
{
	GDTREG gdtreg;
	GDTENT* g;
	short TaskSeg;

	_asm cli; // don't get interrupted!
	_asm sgdt gdtreg; // get GDT address
	_asm str TaskSeg; // get TSS selector index
	g = gdtreg.base + (TaskSeg >> 3); // get ptr to TSS descriptor
	g->limit = size; // modify TSS segment limit

// MUST set selector type field to 9, to indicate the task is
// NOT BUSY.  Otherwise the LTR instruction causes a fault.

	g->type = 9; // mark TSS as "not busy"

// We must do a load of the Task register, else the processor
// never sees the new TSS selector limit.

	_asm ltr TaskSeg; // reload task register (TR)
	_asm sti; // let interrupts continue
}

/**
 * This routine gives total I/O access across the whole system.
 * It does this by modifying the limit of the TSS segment by direct
 * modification of the TSS descriptor entry in the GDT.
 * This descriptor is set up just once at sysetem init time.  Once we
 * modify it, it stays untouched across all processes.
 */
void svgalib_totalio_on(void)
{
	SetTSSLimit(0x20ab + 0xf00);
}

/**
 * This returns the TSS segment to its normal size of 0x20ab, which
 * is two less than the default I/O map base address of 0x20ad.
 */
void svgalib_totalio_off(void)
{
	SetTSSLimit(0x20ab);
}

NTSTATUS svgalibDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION irpStack;
	PVOID ioBuffer;
	ULONG inputBufferLength;
	ULONG outputBufferLength;
	ULONG ioControlCode;
	NTSTATUS status;

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	ioBuffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (irpStack->MajorFunction) {
	case IRP_MJ_CREATE:
		outputBufferLength = 0;
		status = STATUS_SUCCESS;
		break;
	case IRP_MJ_CLOSE:
		outputBufferLength = 0;
		status = STATUS_SUCCESS;
		break;
	case IRP_MJ_DEVICE_CONTROL:
		ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

		switch (ioControlCode) {
		case IOCTL_SVGALIB_MAP :
			status = svgalib_map(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_UNMAP :
			status = svgalib_unmap(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_PORT_READ :
			status = svgalib_port_read(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_PORT_WRITE :
			status = svgalib_port_write(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_PCI_READ :
			status = svgalib_pci_read(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_PCI_WRITE :
			status = svgalib_pci_write(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_PCI_BUS :
			status = svgalib_pci_bus(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
		case IOCTL_SVGALIB_GIVEIO_ON :
			svgalib_giveio_on();
			status = STATUS_SUCCESS;
			break;
		case IOCTL_SVGALIB_GIVEIO_OFF :
			svgalib_giveio_off();
			status = STATUS_SUCCESS;
			break;
		case IOCTL_SVGALIB_TOTALIO_ON :
			svgalib_totalio_on();
			status = STATUS_SUCCESS;
			break;
		case IOCTL_SVGALIB_TOTALIO_OFF :
			svgalib_totalio_off();
			status = STATUS_SUCCESS;
			break;
		default:
			svgalibKdPrint(("SVGAWIN: unknown IRP_MJ_DEVICE_CONTROL\n"));
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		break;
	default:
		svgalibKdPrint(("SVGAWIN: unknown IRP_MJ\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	if (NT_SUCCESS(status)) {
		Irp->IoStatus.Information = outputBufferLength;
	} else {
		Irp->IoStatus.Information = 0;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

VOID svgalibUnload(IN PDRIVER_OBJECT DriverObject)
{
	WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\SVGALIB";
	UNICODE_STRING  deviceLinkUnicodeString;

	if (IOPM_local)
		MmFreeNonCachedMemory(IOPM_local, sizeof(IOPM));

	// delete the symbolic link
	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);
	IoDeleteSymbolicLink(&deviceLinkUnicodeString);

	// delete the device object
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS status;
	WCHAR deviceNameBuffer[] = L"\\Device\\SVGALIB";
	UNICODE_STRING deviceNameUnicodeString;
	WCHAR deviceLinkBuffer[] = L"\\DosDevices\\SVGALIB";
	UNICODE_STRING deviceLinkUnicodeString;

	/* Allocate a buffer for the local IOPM and zero it. */
	IOPM_local = MmAllocateNonCachedMemory(sizeof(IOPM));
	if (IOPM_local == 0)
		return STATUS_INSUFFICIENT_RESOURCES;
	RtlZeroMemory(IOPM_local, sizeof(IOPM));

	RtlInitUnicodeString(&deviceNameUnicodeString, deviceNameBuffer);
	status = IoCreateDevice(DriverObject, 0, &deviceNameUnicodeString, FILE_DEVICE_SVGALIB, 0, TRUE, &deviceObject);

	if (!NT_SUCCESS(status)) {
		svgalibKdPrint(("SVGAWIN: IoCreateDevice failed\n"));
		return status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = svgalibDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = svgalibDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = svgalibDispatch;
	DriverObject->DriverUnload = svgalibUnload;

	// create the symbolic link
	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);
	status = IoCreateSymbolicLink(&deviceLinkUnicodeString, &deviceNameUnicodeString);
	if (!NT_SUCCESS(status)) {
		svgalibKdPrint(("SVGAWIN: IoCreateSymbolicLink failed\n"));
		IoDeleteDevice(deviceObject);
		return status;
	}

	status = STATUS_SUCCESS;
	return status;
}
