#include "ntddk.h"

#include "svgacode.h"

#if 0
NTSTATUS svgalib_map(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
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
		DbgPrint("svgalib: Invalid input or output buffer\n");
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	busNumber = in->bus;
	physicalAddress = in->address;
	inIoSpace1 = 0;
	inIoSpace2 = 0;
	length = in->size;

	/* get a pointer to physical memory... */
	/* - create the name */
	/* - initialize the data to find the object */
	/* - open a handle to the oject and check the status */
	/* - get a pointer to the object */
	/* - free the handle */
	RtlInitUnicodeString(&physicalMemoryUnicodeString, L"\\Device\\PhysicalMemory");

	InitializeObjectAttributes(&objectAttributes, &physicalMemoryUnicodeString, OBJ_CASE_INSENSITIVE, (HANDLE) NULL, (PSECURITY_DESCRIPTOR) NULL);

	status = ZwOpenSection(&physicalMemoryHandle, SECTION_ALL_ACCESS, &objectAttributes);
	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: ZwOpenSection failed\n");
		goto err;
	}

	status = ObReferenceObjectByHandle(physicalMemoryHandle, SECTION_ALL_ACCESS, (POBJECT_TYPE) NULL, KernelMode, &PhysicalMemorySection, (POBJECT_HANDLE_INFORMATION) NULL);

	if (!NT_SUCCESS(status)){
		DbgPrint("svgalib: ObReferenceObjectByHandle failed\n");
		goto err_handle;
	}

	/* initialize the physical addresses that will be translated */
	physicalAddressEnd = RtlLargeIntegerAdd(physicalAddress, RtlConvertUlongToLargeInteger(length));

	/* translate the physical addresses */
	translateBaseAddress = HalTranslateBusAddress(PCIBus, busNumber, physicalAddress, &inIoSpace1, &physicalAddressBase);
	translateEndAddress = HalTranslateBusAddress(PCIBus, busNumber, physicalAddressEnd, &inIoSpace2, &physicalAddressEnd);

	if (!(translateBaseAddress && translateEndAddress)) {
		DbgPrint("svgalib: HalTranslatephysicalAddress failed\n");
		status = STATUS_UNSUCCESSFUL;
		goto err_handle;
	}

	/* calculate the length of the memory to be mapped */
	mappedLength = RtlLargeIntegerSubtract(physicalAddressEnd, physicalAddressBase);

	/* if the mappedlength is zero, something very weird happened in the HAL */
	/* since the Length was checked against zero */
	if (mappedLength.LowPart == 0) {
		DbgPrint("svgalib: mappedLength.LowPart == 0\n");
		status = STATUS_UNSUCCESSFUL;
		goto err_handle;
	}

	length = mappedLength.LowPart;

	/* initialize view base that will receive the physical mapped */
	/* address after the MapViewOfSection call */
	viewBase = physicalAddressBase;

	/* let ZwMapViewOfSection pick an address */
	virtualAddress = NULL;

	/* map the section */
	status = ZwMapViewOfSection(physicalMemoryHandle, (HANDLE)-1, &virtualAddress, 0L, length, &viewBase, &length, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);

	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: ZwMapViewOfSection failed\n");
		goto err_handle;
	}

	/* mapping the section above rounded the physical address down to the */
	/* nearest 64 K boundary. Now return a virtual address that sits where */
	/* we want by adding in the offset from the beginning of the section. */
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

NTSTATUS svgalib_unmap(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_UNMAP_IN* in = (SVGALIB_UNMAP_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_UNMAP_IN) || OutputBufferLength != 0) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	status = ZwUnmapViewOfSection((HANDLE)-1, in->address);
	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: ZwUnmapViewOfSection failed\n");
		goto err;
	}

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

#else

/* Alternate method found in the libdha library (from mplayer) */

/* Reference: Map Adapter RAM into Process Address Space */
/* http://support.microsoft.com/default.aspx?scid=kb;en-us;q189327 */

KSPIN_LOCK map_spin;

struct map_entry {
	PMDL mdl;
	PVOID sys_address;
	PVOID user_address;
	ULONG size;
	struct map_entry* next;
};

static struct map_entry* map_list = 0;

NTSTATUS svgalib_map(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_MAP_IN* in = (SVGALIB_MAP_IN*)IoBuffer;
	SVGALIB_MAP_OUT* out = (SVGALIB_MAP_OUT*)IoBuffer;
	PKIRQL irql;
	PMDL mdl;
	PVOID sys_address;
	struct map_entry* entry;
	MEMORY_CACHING_TYPE cache;
	PVOID user_address;
    
	if (InputBufferLength != sizeof(SVGALIB_MAP_IN) || OutputBufferLength != sizeof(SVGALIB_MAP_OUT)) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
		return STATUS_INVALID_PARAMETER;
	}

	DbgPrint("svgalib: svgalib_map(%x, %u)\n", (unsigned)in->address.LowPart, (unsigned)in->size);

	__try {
		cache = MmNonCached;
		// cache = MmWriteCombined;

		sys_address = MmMapIoSpace(in->address, in->size, cache);
		if (!sys_address){
			DbgPrint("svgalib: MmMapIoSpace failed\n");
			return STATUS_INVALID_PARAMETER;
		}

		mdl = IoAllocateMdl(sys_address, in->size, FALSE, FALSE, NULL);
		if (!mdl) {
			DbgPrint("svgalib: IoAllocateMdl failed\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		MmBuildMdlForNonPagedPool(mdl);

		user_address = (PVOID)(((ULONG)PAGE_ALIGN(MmMapLockedPages(mdl,UserMode))) + MmGetMdlByteOffset(mdl));
		if (!user_address) {
			DbgPrint("svgalib: MmMapLockedPages failed\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

	} __except(EXCEPTION_EXECUTE_HANDLER) {
		NTSTATUS status; 
		status = GetExceptionCode(); 
		DbgPrint("svgalib: svgalib_map failed due to exception 0x%x\n", status);
		return status;       
	}

	entry = MmAllocateNonCachedMemory(sizeof(struct map_entry));

	if (!entry) {
		MmUnmapLockedPages(user_address, mdl); 
		IoFreeMdl(mdl);
		MmUnmapIoSpace(sys_address, in->size);
		DbgPrint("svgalib: MmAllocateNonCachedMemory failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	entry->mdl = mdl;
	entry->sys_address = sys_address;
	entry->user_address = user_address;
	entry->size = in->size;

	KeAcquireSpinLock(&map_spin, &irql);

	entry->next = map_list;
	map_list = entry;

	KeReleaseSpinLock(&map_spin, irql);

	DbgPrint("svgalib: svgalib_map(%x, %u) -> %x\n", (unsigned)in->address.LowPart, (unsigned)in->size, (unsigned)user_address);

	out->address = user_address;
 
	return STATUS_SUCCESS;
}

NTSTATUS svgalib_unmap(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_UNMAP_IN* in = (SVGALIB_UNMAP_IN*)IoBuffer;
	PKIRQL irql;
	NTSTATUS status;
	struct map_entry* entry;

	if (InputBufferLength != sizeof(SVGALIB_UNMAP_IN) || OutputBufferLength != 0) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
		return STATUS_INVALID_PARAMETER;
	}

	DbgPrint("svgalib: svgalib_unmap(%x)\n", (unsigned)in->address);

	KeAcquireSpinLock(&map_spin, &irql);

	if (!map_list) {
		entry = 0;
	} else if (map_list->user_address == in->address) {
		entry = map_list;
		map_list = entry->next;
	} else {
		struct map_entry* pivot = map_list;
		while (pivot->next && pivot->next->user_address != in->address) {
			pivot = pivot->next;
		}
		if (!pivot->next) {
			entry = 0;
		} else {
			entry = pivot->next;
			pivot->next = pivot->next->next;
		}
	}

	KeReleaseSpinLock(&map_spin, irql);

	if (!entry) {
		DbgPrint("svgalib: svgalib_unmap entry not found\n");
		return STATUS_INVALID_PARAMETER;
	}

	__try {
		MmUnmapLockedPages(entry->user_address, entry->mdl);
		IoFreeMdl(entry->mdl);
		MmUnmapIoSpace(entry->sys_address, entry->size);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		NTSTATUS status;
		status = GetExceptionCode();
		DbgPrint("svgalib: svgalib_unmap failed due to exception 0x%x\n", status);
		return status;
	}

	DbgPrint("svgalib: svgalib_unmap(%x) -> ok\n", (unsigned)in->address);

	MmFreeNonCachedMemory(entry, sizeof(struct map_entry));

	return STATUS_SUCCESS;
}
#endif

unsigned char inportb(unsigned _port) {
	unsigned char rv;
	__asm mov edx, [_port]
	__asm in al, dx
	__asm mov [rv], al
	return rv;
}

unsigned short inportw(unsigned _port) {
	unsigned short rv;
	__asm mov edx, [_port]
	__asm in ax, dx
	__asm mov [rv], ax
	return rv;
}

unsigned inportl(unsigned _port) {
	unsigned rv;
	__asm mov edx, [_port]
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

NTSTATUS svgalib_port_read(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PORT_READ_IN* in = (SVGALIB_PORT_READ_IN*)IoBuffer;
	SVGALIB_PORT_READ_OUT* out = (SVGALIB_PORT_READ_OUT*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PORT_READ_IN) || OutputBufferLength != sizeof(SVGALIB_PORT_READ_OUT)) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
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

NTSTATUS svgalib_port_write(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PORT_WRITE_IN* in = (SVGALIB_PORT_WRITE_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PORT_WRITE_IN) || OutputBufferLength != 0) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
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

NTSTATUS svgalib_pci_bus(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_BUS_OUT* out = (SVGALIB_PCI_BUS_OUT*)IoBuffer;
	NTSTATUS status;
	ULONG bus;
	UCHAR data[4];

	if (InputBufferLength != 0 || OutputBufferLength != sizeof(SVGALIB_PCI_BUS_OUT)) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
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

NTSTATUS svgalib_pci_read(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_READ_IN* in = (SVGALIB_PCI_READ_IN*)IoBuffer;
	SVGALIB_PCI_READ_OUT* out = (SVGALIB_PCI_READ_OUT*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PCI_READ_IN) || OutputBufferLength != sizeof(SVGALIB_PCI_READ_OUT)) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
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

NTSTATUS svgalib_pci_write(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_PCI_WRITE_IN* in = (SVGALIB_PCI_WRITE_IN*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != sizeof(SVGALIB_PCI_WRITE_IN) || OutputBufferLength != 0) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
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
 * This is the "structure" of the IOPM. It is just a simple
 * character array of length 0x2000.
 *
 * This holds 8K * 8 bits -> 64K bits of the IOPM, which maps the
 * entire 64K I/O space of the x86 processor. Any 0 bits will give
 * access to the corresponding port for user mode processes. Any 1
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
 * the newly copied map is actually used. Otherwise, the IOPM offset
 * points beyond the end of the TSS segment limit, causing any I/O
 * access by the user mode process to generate an exception.
 */
void Ke386SetIoAccessMap(int, IOPM*);
void Ke386QueryIoAccessMap(int, IOPM*);
void Ke386IoSetAccessProcess(PEPROCESS, int);

/***
 * Set the IOPM (I/O permission map) of the calling process so that it
 * is given full I/O access. Our IOPM_local[] array is all zeros, so
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
 * descriptor entry. Documented in Intel processor handbooks.
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
 * limit field. We get the selector ID from the STR instruction,
 * index into the GDT, and poke in the new limit. In order for the
 * new limit to take effect, we must then read the task segment
 * selector back into the task register (TR).
 */
void SetTSSLimit(int size)
{
	GDTREG gdtreg;
	GDTENT* g;
	short TaskSeg;

	_asm cli; /* don't get interrupted! */
	_asm sgdt gdtreg; /* get GDT address */
	_asm str TaskSeg; /* get TSS selector index */
	g = gdtreg.base + (TaskSeg >> 3); /* get ptr to TSS descriptor */
	g->limit = size; /* modify TSS segment limit */

/* MUST set selector type field to 9, to indicate the task is */
/* NOT BUSY. Otherwise the LTR instruction causes a fault. */

	g->type = 9; /* mark TSS as "not busy" */

/* We must do a load of the Task register, else the processor */
/* never sees the new TSS selector limit. */

	_asm ltr TaskSeg; /* reload task register (TR) */
	_asm sti; /* let interrupts continue */
}

/**
 * This routine gives total I/O access across the whole system.
 * It does this by modifying the limit of the TSS segment by direct
 * modification of the TSS descriptor entry in the GDT.
 * This descriptor is set up just once at sysetem init time. Once we
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

NTSTATUS svgalib_video_ioctl(PDEVICE_OBJECT DeviceObject, ULONG IoControlCode, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength, ULONG* ResultBufferLength)
{
	PIRP irp;
	PIO_STACK_LOCATION stack;
	IO_STATUS_BLOCK status_block;
	KEVENT event;
	NTSTATUS status;
	DEVICE_OBJECT* video_device;
	UNICODE_STRING video_name;

	RtlInitUnicodeString(&video_name, L"\\Device\\Video0");

	status = IoAttachDevice(DeviceObject, &video_name, &video_device);
	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: IoAttachDevice failed\n");
		goto err;
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(
		IRP_MJ_DEVICE_CONTROL,
		video_device,
		InputBufferLength != 0 ? IoBuffer : 0,
		InputBufferLength,
		OutputBufferLength != 0 ? IoBuffer : 0,
		OutputBufferLength,
		FALSE,
		&event,
		&status_block
	);

	if (irp == 0) {
		DbgPrint("svgalib: IoBuildDeviceIoControlRequest failed\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto err_file;
	}

	stack = IoGetNextIrpStackLocation(irp);
	stack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
	irp->AssociatedIrp.SystemBuffer = IoBuffer;

	status = IoCallDriver(video_device, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event,Executive,KernelMode,TRUE,NULL);
		status = status_block.Status;
	}

	*ResultBufferLength = status_block.Information;

	IoDetachDevice(video_device);
	return status;

err_file:
	IoDetachDevice(video_device);
err:
	return status;
}

NTSTATUS svgalib_version(PDEVICE_OBJECT DeviceObject, PVOID IoBuffer, ULONG InputBufferLength, ULONG OutputBufferLength)
{
	SVGALIB_VERSION_OUT* out = (SVGALIB_VERSION_OUT*)IoBuffer;
	NTSTATUS status;

	if (InputBufferLength != 0 || OutputBufferLength != sizeof(SVGALIB_VERSION_OUT)) {
		DbgPrint("svgalib: Invalid input or output buffer\n");
		status = STATUS_INVALID_PARAMETER;
		goto err;
	}

	out->version = SVGALIB_VERSION;

	status = STATUS_SUCCESS;
	return status;

err:
	return status;
}

NTSTATUS svgalibDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpStack;
	PVOID ioBuffer;
	ULONG inputBufferLength;
	ULONG outputBufferLength;
	ULONG ioControlCode;
	ULONG device_code;
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
		case IOCTL_SVGALIB_VERSION :
			status = svgalib_version(DeviceObject, ioBuffer, inputBufferLength, outputBufferLength);
			break;
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
			device_code = (irpStack->Parameters.DeviceIoControl.IoControlCode >> 16) & 0xFFFF;
			if (device_code == FILE_DEVICE_SVGALIB) {
				/* adjust the ioctl with FILE_DEVICE_VIDEO */
				ioControlCode = (irpStack->Parameters.DeviceIoControl.IoControlCode & 0x0000FFFF) | ((ULONG)FILE_DEVICE_VIDEO << 16);
				status = svgalib_video_ioctl(DeviceObject, ioControlCode, ioBuffer, inputBufferLength, outputBufferLength, &outputBufferLength);
			} else {
				DbgPrint("svgalib: unknown IRP_MJ_DEVICE_CONTROL\n");
				status = STATUS_INVALID_DEVICE_REQUEST;
			}
			break;
		}

		break;
	default:
		DbgPrint("svgalib: unknown IRP_MJ\n");
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

VOID svgalibUnload(PDRIVER_OBJECT DriverObject)
{
	WCHAR deviceLinkBuffer[] = L"\\DosDevices\\SVGALIB";
	UNICODE_STRING deviceLinkUnicodeString;

	/* delete the symbolic link */
	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);
	IoDeleteSymbolicLink(&deviceLinkUnicodeString);

	/* delete the device object */
	IoDeleteDevice(DriverObject->DeviceObject);

	if (IOPM_local)
		MmFreeNonCachedMemory(IOPM_local, sizeof(IOPM));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS status;
	WCHAR deviceNameBuffer[] = L"\\Device\\SVGALIB";
	UNICODE_STRING deviceNameUnicodeString;
	WCHAR deviceLinkBuffer[] = L"\\DosDevices\\SVGALIB";
	UNICODE_STRING deviceLinkUnicodeString;

	DbgPrint("svgalib: load\n");

	/* Allocate a buffer for the local IOPM and zero it. */
	IOPM_local = MmAllocateNonCachedMemory(sizeof(IOPM));
	if (IOPM_local == 0) {
		DbgPrint("svgalib: MmAllocateNonCachedMemory failed\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	RtlZeroMemory(IOPM_local, sizeof(IOPM));

	KeInitializeSpinLock(&map_spin);

	RtlInitUnicodeString(&deviceNameUnicodeString, deviceNameBuffer);
	status = IoCreateDevice(DriverObject, 0, &deviceNameUnicodeString, FILE_DEVICE_SVGALIB, 0, FALSE, &deviceObject);
	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: IoCreateDevice failed\n");
		return status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = svgalibDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = svgalibDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = svgalibDispatch;
	DriverObject->DriverUnload = svgalibUnload;

	/* create the symbolic link */
	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);
	status = IoCreateSymbolicLink(&deviceLinkUnicodeString, &deviceNameUnicodeString);
	if (!NT_SUCCESS(status)) {
		DbgPrint("svgalib: IoCreateSymbolicLink failed\n");
		IoDeleteDevice(deviceObject);
		return status;
	}

	status = STATUS_SUCCESS;
	return status;
}

