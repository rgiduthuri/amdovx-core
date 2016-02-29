/* 
Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include "ago_internal.h"
#include <math.h>
#include <sstream>

// global locks
static vx_bool g_cs_context_initialized = vx_false_e;
static CRITICAL_SECTION g_cs_context;

// enumeration constants
static struct { const char * name; vx_enum value; vx_size size; } s_table_constants[] = {
		{ "CHANNEL_0", VX_CHANNEL_0 },
		{ "CHANNEL_1", VX_CHANNEL_1 },
		{ "CHANNEL_2", VX_CHANNEL_2 },
		{ "CHANNEL_3", VX_CHANNEL_3 },
		{ "CHANNEL_R", VX_CHANNEL_R },
		{ "CHANNEL_G", VX_CHANNEL_G },
		{ "CHANNEL_B", VX_CHANNEL_B },
		{ "CHANNEL_A", VX_CHANNEL_A },
		{ "CHANNEL_Y", VX_CHANNEL_Y },
		{ "CHANNEL_U", VX_CHANNEL_U },
		{ "CHANNEL_V", VX_CHANNEL_V },
		{ "WRAP", VX_CONVERT_POLICY_WRAP },
		{ "SATURATE", VX_CONVERT_POLICY_SATURATE },
		{ "NEAREST_NEIGHBOR", VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR },
		{ "BILINEAR", VX_INTERPOLATION_TYPE_BILINEAR },
		{ "AREA", VX_INTERPOLATION_TYPE_AREA },
		{ "BINARY", VX_THRESHOLD_TYPE_BINARY },
		{ "RANGE", VX_THRESHOLD_TYPE_RANGE },
		{ "NORM_L1", VX_NORM_L1 },
		{ "NORM_L2", VX_NORM_L2 },
		{ "ROUND_POLICY_TO_ZERO", VX_ROUND_POLICY_TO_ZERO },
		{ "ROUND_POLICY_TO_NEAREST_EVEN", VX_ROUND_POLICY_TO_NEAREST_EVEN },
		{ "CRITERIA_ITERATIONS", VX_TERM_CRITERIA_ITERATIONS },
		{ "CRITERIA_EPSILON", VX_TERM_CRITERIA_EPSILON },
		{ "CRITERIA_BOTH", VX_TERM_CRITERIA_BOTH },
		{ "BORDER_MODE_UNDEFINED", VX_BORDER_MODE_UNDEFINED },
		{ "BORDER_MODE_REPLICATE", VX_BORDER_MODE_REPLICATE },
		{ "BORDER_MODE_CONSTANT", VX_BORDER_MODE_CONSTANT },
		{ "VX_DIRECTIVE_DISABLE_LOGGING", VX_DIRECTIVE_DISABLE_LOGGING },
		{ "VX_DIRECTIVE_ENABLE_LOGGING", VX_DIRECTIVE_ENABLE_LOGGING },
		{ "VX_DIRECTIVE_READ_ONLY", VX_DIRECTIVE_AMD_READ_ONLY },
		{ "RECTANGLE", VX_TYPE_RECTANGLE, sizeof(vx_rectangle_t) },
		{ "KEYPOINT", VX_TYPE_KEYPOINT, sizeof(vx_keypoint_t) },
		{ "COORDINATES2D", VX_TYPE_COORDINATES2D, sizeof(vx_coordinates2d_t) },
		{ "COORDINATES3D", VX_TYPE_COORDINATES3D, sizeof(vx_coordinates3d_t) },
		{ "DF_IMAGE", VX_TYPE_DF_IMAGE, sizeof(vx_df_image) },
		{ "ENUM", VX_TYPE_ENUM, sizeof(vx_enum) },
		{ "UINT64", VX_TYPE_UINT64, sizeof(vx_uint64) },
		{ "INT64", VX_TYPE_INT64, sizeof(vx_int64) },
		{ "UINT32", VX_TYPE_UINT32, sizeof(vx_uint32) },
		{ "INT32", VX_TYPE_INT32, sizeof(vx_int32) },
		{ "UINT16", VX_TYPE_UINT16, sizeof(vx_uint16) },
		{ "INT16", VX_TYPE_INT16, sizeof(vx_int16) },
		{ "UINT8", VX_TYPE_UINT8, sizeof(vx_uint8) },
		{ "INT8", VX_TYPE_INT8, sizeof(vx_int8) },
		{ "FLOAT32", VX_TYPE_FLOAT32, sizeof(vx_float32) },
		{ "FLOAT64", VX_TYPE_FLOAT64, sizeof(vx_float64) },
		{ "SIZE", VX_TYPE_SIZE, sizeof(vx_size) },
		{ "BOOL", VX_TYPE_BOOL, sizeof(vx_bool) },
		{ "KEYPOINT_XYS", AGO_TYPE_KEYPOINT_XYS, sizeof(ago_keypoint_xys_t) },
		{ "STRING", VX_TYPE_STRING_AMD },
		// for debug purposes only
		{ "VX_TYPE_LUT", VX_TYPE_LUT },
		{ "VX_TYPE_DISTRIBUTION", VX_TYPE_DISTRIBUTION },
		{ "VX_TYPE_PYRAMID", VX_TYPE_PYRAMID },
		{ "VX_TYPE_THRESHOLD", VX_TYPE_THRESHOLD },
		{ "VX_TYPE_MATRIX", VX_TYPE_MATRIX },
		{ "VX_TYPE_CONVOLUTION", VX_TYPE_CONVOLUTION },
		{ "VX_TYPE_SCALAR", VX_TYPE_SCALAR },
		{ "VX_TYPE_ARRAY", VX_TYPE_ARRAY },
		{ "VX_TYPE_IMAGE", VX_TYPE_IMAGE },
		{ "VX_TYPE_REMAP", VX_TYPE_REMAP },
		{ "VX_TYPE_STRING", VX_TYPE_STRING_AMD },
		{ "AGO_TYPE_MEANSTDDEV_DATA", AGO_TYPE_MEANSTDDEV_DATA },
		{ "AGO_TYPE_MINMAXLOC_DATA", AGO_TYPE_MINMAXLOC_DATA },
		{ "AGO_TYPE_CANNY_STACK", AGO_TYPE_CANNY_STACK },
		{ "AGO_TYPE_SCALE_MATRIX", AGO_TYPE_SCALE_MATRIX },
		{ NULL, 0 }
};

const char * agoEnum2Name(vx_enum e)
{
	for (vx_uint32 i = 0; s_table_constants[i].name; i++) {
		if (s_table_constants[i].value == e) 
			return s_table_constants[i].name;
	}
	return NULL;
}

size_t agoType2Size(vx_context context, vx_enum type)
{
	for (vx_uint32 i = 0; s_table_constants[i].name; i++) {
		if (s_table_constants[i].value == type)
			return s_table_constants[i].size;
	}
	if (context) {
		return agoGetUserStructSize(context, type);
	}
	return 0;
}

int agoChannelEnum2Index(vx_enum channel)
{
	int index = -1;
	if (channel >= VX_CHANNEL_0 && channel <= VX_CHANNEL_3)
		index = channel - VX_CHANNEL_0;
	else if (channel >= VX_CHANNEL_R && channel <= VX_CHANNEL_A)
		index = channel - VX_CHANNEL_R;
	else if (channel >= VX_CHANNEL_Y && channel <= VX_CHANNEL_V)
		index = channel - VX_CHANNEL_Y;
	return index;
}

vx_enum agoName2Enum(const char * name)
{
	for (vx_uint32 i = 0; s_table_constants[i].name; i++) {
		if (!strcmp(name, s_table_constants[i].name))
			return s_table_constants[i].value;
	}
	return 0;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const char * s_, char delim) {
	const std::string s(s_);
	return split(s, delim);
}

void agoLockGlobalContext()
{
	if (!g_cs_context_initialized) {
		InitializeCriticalSection(&g_cs_context);
		g_cs_context_initialized = vx_true_e;
	}
	EnterCriticalSection(&g_cs_context);
}

void agoUnlockGlobalContext()
{
	LeaveCriticalSection(&g_cs_context);
}

void * agoAllocMemory(vx_size size)
{
	// to keep track of allocations
	static vx_int32 s_ago_alloc_id_count = 0;
	// make the buffer allocation 256-bit aligned and add header for debug
	vx_size size_alloc = size;
	if (size_alloc & 31) size_alloc += 32 - (size_alloc & 31);
	size_alloc += sizeof(vx_uint32) + sizeof(AgoAllocInfo) + 32 + AGO_MEMORY_ALLOC_EXTRA_PADDING + AGO_MEMORY_ALLOC_EXTRA_PADDING;
	if (size_alloc & 31) size_alloc += 32 - (size_alloc & 31);
	vx_uint8 * mem = (vx_uint8 *)calloc(1, size_alloc); if (!mem) return nullptr;
	((vx_uint32 *)mem)[0] = 0xfadedcab; // marker for debug
	vx_uint8 * mem_aligned = mem + sizeof(vx_uint32) + sizeof(AgoAllocInfo) + AGO_MEMORY_ALLOC_EXTRA_PADDING;
	mem_aligned += ((((size_t)mem_aligned) & 31) ? (32 - (((size_t)mem_aligned) & 31)) : 0);
	AgoAllocInfo * mem_info = &((AgoAllocInfo *)(mem_aligned - AGO_MEMORY_ALLOC_EXTRA_PADDING))[-1];
	mem_info->allocated = mem;
	mem_info->requested_size = size;
	mem_info->retain_count = 1;
	mem_info->allocate_id = s_ago_alloc_id_count++;
	return mem_aligned;
}

void agoRetainMemory(void * mem)
{
	AgoAllocInfo * mem_info = &((AgoAllocInfo *)((vx_uint8 *)mem - AGO_MEMORY_ALLOC_EXTRA_PADDING))[-1];
	// increment retain_count
	mem_info->retain_count++;
}

void agoReleaseMemory(void * mem)
{
	AgoAllocInfo * mem_info = &((AgoAllocInfo *)((vx_uint8 *)mem - AGO_MEMORY_ALLOC_EXTRA_PADDING))[-1];
	// decrement retain_count
	mem_info->retain_count--;
	if (((vx_uint32 *)mem_info->allocated)[0] != 0xfadedcab) {
		agoAddLogEntry(NULL, VX_SUCCESS, "WARNING: agoReleaseMemory: invalid pointer\n");
	}
	else if (mem_info->retain_count < 0) {
		agoAddLogEntry(NULL, VX_SUCCESS, "WARNING: agoReleaseMemory: detected retain_count=%d for allocate_id=%d with size=%d\n", mem_info->retain_count, mem_info->allocate_id, (vx_uint32)mem_info->requested_size);
	}
	else if (mem_info->retain_count == 0) {
		// free the allocated pointer
		free(mem_info->allocated);
	}
}

void agoResetReference(AgoReference * ref, vx_enum type, vx_context context, vx_reference scope)
{
	ref->magic = AGO_MAGIC_VALID;
	ref->type = type;
	ref->context = context;
	ref->scope = scope;
	ref->external_count = 0;
	ref->internal_count = 0;
	ref->read_count = 0;
	ref->write_count = 0;
	ref->enable_logging = ENABLE_LOG_MESSAGES_DEFAULT;
	if (context) ref->enable_logging = context->ref.enable_logging;
	if (scope) ref->enable_logging = scope->enable_logging;
}

void agoAddData(AgoDataList * dataList, AgoData * data)
{
	if (dataList->tail) dataList->tail->next = data;
	else dataList->head = data;
	dataList->tail = data;
	dataList->count++;
}

void agoAddNode(AgoNodeList * nodeList, AgoNode * node)
{
	if (nodeList->tail) nodeList->tail->next = node;
	else nodeList->head = node;
	nodeList->tail = node;
	nodeList->count++;
}

void agoAddKernel(AgoKernelList * kernelList, AgoKernel * kernel)
{
	if (kernelList->tail) kernelList->tail->next = kernel;
	else kernelList->head = kernel;
	kernelList->tail = kernel;
	kernelList->count++;
}

void agoAddGraph(AgoGraphList * graphList, AgoGraph * graph)
{
	if (graphList->tail) graphList->tail->next = graph;
	else graphList->head = graph;
	graphList->tail = graph;
	graphList->count++;
}

AgoGraph * agoRemoveGraph(AgoGraphList * list, AgoGraph * item)
{
	if (list->head == item) {
		if (list->tail == item)
			list->head = list->tail = NULL;
		else
			list->head = item->next;
		list->count--;
		item->next = 0;
		return item;
	}
	else {
		for (AgoGraph * cur = list->head; cur->next; cur = cur->next) {
			if (cur->next == item) {
				if (list->tail == item)
					list->tail = cur;
				cur->next = item->next;
				list->count--;
				item->next = 0;
				return item;
			}
		}
	}
	return 0;
}

AgoKernel * agoRemoveKernel(AgoKernelList * list, AgoKernel * item)
{
	if (list->head == item) {
		if (list->tail == item)
			list->head = list->tail = NULL;
		else
			list->head = item->next;
		list->count--;
		item->next = 0;
		return item;
	}
	else {
		for (AgoKernel * cur = list->head; cur->next; cur = cur->next) {
			if (cur->next == item) {
				if (list->tail == item)
					list->tail = cur;
				cur->next = item->next;
				list->count--;
				item->next = 0;
				return item;
			}
		}
	}
	return 0;
}

int agoRemoveNode(AgoNodeList * list, AgoNode * item, bool moveToTrash)
{
	int status = -1;
	if (!item) {
		return status;
	}
	if (list->head) {
		if (list->head == item) {
			if (list->tail == item)
				list->head = list->tail = NULL;
			else
				list->head = item->next;
			list->count--;
			item->next = 0;
			status = 0;
		}
		else {
			for (AgoNode * cur = list->head; cur->next; cur = cur->next) {
				if (cur->next == item) {
					if (list->tail == item)
						list->tail = cur;
					cur->next = item->next;
					list->count--;
					item->next = 0;
					status = 0;
					break;
				}
			}
		}
	}
	if (status != 0) {
		// check in trash, if it has no external references
		if (list->trash) {
			for (AgoNode * cur = list->trash; cur == list->trash || cur->next; cur = cur->next) {
				if (cur == item || cur->next == item) {
					if (cur == item) list->trash = item->next;
					else cur->next = item->next;
					list->count--;
					item->next = 0;
					status = 0;
					break;
				}
			}
		}
	}
	if (status == 0) {
		if (moveToTrash) {
			// still has external references, so keep into trash
			item->ref.internal_count = 0;
			item->next = list->trash;
			list->trash = item;
		}
		else {
			// not needed anymore, just release it
			delete item;
		}
	}
	return status;
}

int agoRemoveData(AgoDataList * list, AgoData * item, AgoData ** trash)
{
	int status = -1;
	if (!item) {
		return status;
	}
	if (list->head == item) {
		if (list->tail == item)
			list->head = list->tail = NULL;
		else
			list->head = item->next;
		list->count--;
		item->next = 0;
		status = 0;
	}
	else if (list->head) {
		for (AgoData * cur = list->head; cur->next; cur = cur->next) {
			if (cur->next == item) {
				if (list->tail == item)
					list->tail = cur;
				cur->next = item->next;
				list->count--;
				item->next = 0;
				status = 0;
				break;
			}
		}
	}
	if (status != 0) {
		// check in trash
		if (list->trash) {
			for (AgoData * cur = list->trash; cur && (cur == list->trash || cur->next); cur = cur->next) {
				if (cur == item || cur->next == item) {
					if (cur == item) list->trash = item->next;
					else cur->next = item->next;
					list->count--;
					item->next = 0;
					status = 0;
					break;
				}
			}
		}
	}
	if (status == 0) {
		if (trash) {
			// keep in trash
			item->next = *trash;
			*trash = item;
		}
		else {
			// remove parent/child cross references in existing list
			for (int i = 0; i < 2; i++) {
				for (AgoData * data = i ? list->trash : list->head; data; data = data->next) {
					if (data->parent == item)
						data->parent = nullptr;
					for (vx_uint32 child = 0; child < data->numChildren; child++)
						if (data->children[child] == item)
							data->children[child] = nullptr;
				}
			}
			// not needed anymore, just release it
			delete item;
		}
	}
	return status;
}

int agoRemoveDataTree(AgoDataList * list, AgoData * item, AgoData ** trash)
{
	for (vx_uint32 child = 0; child < item->numChildren; child++) {
		if (item->children[child]) {
			if (agoRemoveDataTree(list, item->children[child], trash) < 0)
				return -1;
			item->children[child] = nullptr;
		}
	}
	return agoRemoveData(list, item, trash);
}

void agoRemoveDataInGraph(AgoGraph * agraph, AgoData * adata)
{
#if ENABLE_DEBUG_MESSAGES
	char name[256];
	agoGetDataName(name, adata);
	debug_printf("INFO: agoRemoveDataInGraph: removing data %s\n", name[0] ? name : "<?>");
#endif
	// clear parent link in it's children and give them a name, when missing
	if (adata->children) {
		char dataName[256];
		agoGetDataName(dataName, adata);
		for (vx_uint32 i = 0; dataName[i]; i++) {
			if (dataName[i] == '[' || dataName[i] == ']')
				dataName[i] = '!';
		}
		for (vx_uint32 i = 0; i < adata->numChildren; i++) {
			if (adata->children[i]) {
				// make sure that adata is the owner parent, before clearing the parent link
				if (adata->children[i]->parent == adata) {
					if (dataName[0] && !adata->children[i]->name.length()) {
						char nameChild[256];
						sprintf(nameChild, "%s!%d!", dataName, i);
						adata->children[i]->name = nameChild;
					}
					adata->children[i]->parent = NULL;
				}
			}
		}
	}
	// clear child link in it's paren link
	if (adata->parent) {
		for (vx_uint32 i = 0; i < adata->parent->numChildren; i++) {
			if (adata->parent->children[i] == adata) {
				adata->parent->children[i] = NULL;
			}
		}
	}
	// move the virtual data to trash
	if (agoRemoveData(&agraph->dataList, adata, &agraph->dataList.trash)) {
		char name[256];
		agoGetDataName(name, adata);
		agoAddLogEntry(&adata->ref, VX_FAILURE, "ERROR: agoRemoveDataInGraph: agoRemoveData(*,%s) failed\n", name[0] ? name : "<?>");
	}
}

void agoReplaceDataInGraph(AgoGraph * agraph, AgoData * dataFind, AgoData * dataReplace)
{
	// replace all references to dataFind in the node parameters with dataReplace
	for (AgoNode * anode = agraph->nodeList.head; anode; anode = anode->next) {
		for (vx_uint32 arg = 0; arg < anode->paramCount; arg++) {
			if (anode->paramList[arg]) {
				if (anode->paramList[arg] == dataFind) {
					anode->paramList[arg] = dataReplace;
				}
			}
		}
	}

	// replace all ROI master links
	for (AgoData * adata = agraph->dataList.head; adata; adata = adata->next) {
		if (adata->ref.type == VX_TYPE_IMAGE && adata->u.img.isROI && adata->u.img.roiMasterImage == dataFind) {
			adata->u.img.roiMasterImage = dataReplace;
		}
	}

	//
	// remove dataFind from graph and replaces its links with dataReplace
	//

	// replace parent link in it's children and give them a name, when missing
	if (dataFind->children) {
		char dataName[256];
		agoGetDataName(dataName, dataFind);
		for (vx_uint32 i = 0; dataName[i]; i++) {
			if (dataName[i] == '[' || dataName[i] == ']')
				dataName[i] = '!';
		}
		for (vx_uint32 i = 0; i < dataFind->numChildren; i++) {
			if (dataFind->children[i]) {
				if (dataName[0] && !dataFind->children[i]->name.length()) {
					char nameChild[256];
					sprintf(nameChild, "%s!%d!", dataName, i);
					dataFind->children[i]->name = nameChild;
				}
				dataFind->children[i]->parent = dataReplace;
			}
		}
	}
	// replace child link in it's parent link with dataReplace
	bool removed = false;
	if (dataFind->parent) {
		bool found_in_parent = false;
		for (vx_uint32 i = 0; i < dataFind->parent->numChildren; i++) {
			if (dataFind->parent->children[i] == dataFind) {
				dataFind->parent->children[i] = dataReplace;
				found_in_parent = true;
			}
		}
		if (found_in_parent) {
			// TBD: need to handle the case if vxVerifyGraph is called second time with changes to the graph
			agoRemoveData(&agraph->dataList, dataFind, &agraph->ref.context->graph_garbage_data);
			removed = true;
		}
	}
	if (!removed) {
		// move the virtual data into trash
		if (agoRemoveDataTree(&agraph->dataList, dataFind, &agraph->dataList.trash)) {
			char name[256];
			agoGetDataName(name, dataFind);
			agoAddLogEntry(&agraph->ref, VX_FAILURE, "ERROR: agoReplaceDataInGraph: agoRemoveDataTree(*,%s) failed\n", name[0] ? name : "<?>");
		}
	}
}

int agoShutdownNode(AgoNode * node)
{
	vx_status status = VX_SUCCESS;
	if (node->initialized) {
		AgoKernel * kernel = node->akernel;
		if (kernel) {
			if (kernel->func) {
				status = kernel->func(node, ago_kernel_cmd_shutdown);
			}
			else if (kernel->deinitialize_f) {
				status = kernel->deinitialize_f(node, (vx_reference *)node->paramList, node->paramCount);
			}
			if (status) {
				return status;
			}
			node->akernel = nullptr;
		}
		if (node->localDataPtr_allocated) {
			agoReleaseMemory(node->localDataPtr_allocated);
			node->localDataPtr_allocated = nullptr;
		}
		node->initialized = false;
	}
	return status;
}

void agoResetDataList(AgoDataList * dataList)
{
	for (int i = 0; i < 2; i++) {
		for (AgoData * data = i ? dataList->trash : dataList->head; data;) {
			// save next item pointer
			AgoData * next = data->next;
			// release current item
			delete data;
			// proceed to next item
			data = next;
		}
	}
	memset(dataList, 0, sizeof(*dataList));
}

void agoResetNodeList(AgoNodeList * nodeList)
{
	for (int i = 0; i < 2; i++) {
		for (AgoNode * node = i ? nodeList->trash : nodeList->head; node;) {
			// save next item pointer
			AgoNode * next = node->next;
			// release current item
			delete node;
			// proceed to next item
			node = next;
		}
	}
	memset(nodeList, 0, sizeof(*nodeList));
}

void agoResetKernelList(AgoKernelList * kernelList)
{
	for (AgoKernel * kernel = kernelList->head; kernel;) {
		// save next item pointer
		AgoKernel * next = kernel->next;
		// release current item
		delete kernel;
		// proceed to next item
		kernel = next;
	}
	memset(kernelList, 0, sizeof(*kernelList));
}

static void agoResetSuperNodeList(AgoSuperNode * supernodeList)
{
	for (AgoSuperNode * supernode = supernodeList; supernode;) {
		// save next item pointer
		AgoSuperNode * next = supernode->next;
		// release current item
#if ENABLE_OPENCL
		agoGpuOclReleaseSuperNode(supernode);
#endif
		// TBD: agoResetSuperNode(supernode);
		delete supernode;
		// proceed to next item
		supernode = next;
	}
}

AgoKernel * agoFindKernelByEnum(AgoContext * acontext, vx_enum kernel_id)
{
	// search context
	for (AgoKernel * kernel = acontext->kernelList.head; kernel; kernel = kernel->next) {
		if (kernel->id == kernel_id) return kernel;
	}
	return 0;
}

AgoKernel * agoFindKernelByName(AgoContext * acontext, const vx_char * name)
{
	// search context
	for (AgoKernel * kernel = acontext->kernelList.head; kernel; kernel = kernel->next) {
		if (!strcmp(kernel->name, name)) return kernel;
	}
	if (!strstr(name, ".")) {
		char fullName[VX_MAX_KERNEL_NAME];
		// search for org.khronos.openvx.<name>
		sprintf(fullName, "org.khronos.openvx.%s", name);
		for (AgoKernel * kernel = acontext->kernelList.head; kernel; kernel = kernel->next) {
			if (!strcmp(kernel->name, fullName)) return kernel;
		}
		// search for org.amd.openvx.<name>
		sprintf(fullName, "com.amd.openvx.%s", name);
		for (AgoKernel * kernel = acontext->kernelList.head; kernel; kernel = kernel->next) {
			if (!strcmp(kernel->name, fullName)) return kernel;
		}
	}
	return 0;
}

AgoData * agoFindDataByName(AgoContext * acontext, AgoGraph * agraph, vx_char * name)
{
	// check for <object>[index] syntax
	char actualName[256]; strcpy(actualName, name);
	int index[4] = { -1, -1, -1, -1 }; // index >=0 indicates special object
	const char * s = strstr(name, "[");
	if (s && name[strlen(name) - 1] == ']') {
		actualName[s - name] = 0;
		for (int i = 0; i < 4 && *s == '['; i++) {
			s++; if (*s == '-') s++;
			index[i] = atoi(s);
			for (; *s != ']'; s++) {
				if (!(*s >= '0' || *s <= '9')) return NULL;
			}
			if (*s != ']') return NULL;
			s++;
		}
	}
	// search graph
	AgoData * data = NULL;
	if (agraph) {
		for (data = agraph->dataList.head; data; data = data->next) {
			if (!strcmp(data->name.c_str(), actualName)) break;
		}
	}
	if (!data) {
		// search context
		for (data = acontext->dataList.head; data; data = data->next) {
			if (!strcmp(data->name.c_str(), actualName)) break;
		}
	}
	if(data) {
		for (int i = 0; i < 4 && index[i] >= 0; i++) {
			if (index[i] < (int)data->numChildren) {
				data = data->children[index[i]];
			}
			else return NULL;
		}
	}
	return data;
}

void agoMarkChildrenAsPartOfDelay(AgoData * adata)
{
	// recursively mark children as part of delay
	for (vx_uint32 child = 0; child < adata->numChildren; child++) {
		if (adata->children[child]) {
			adata->children[child]->isDelayed = vx_true_e;
			agoMarkChildrenAsPartOfDelay(adata->children[child]);
		}
	}
}

bool agoIsPartOfDelay(AgoData * adata)
{
	return adata->isDelayed ? true : false;
}

AgoData * agoGetSiblingTraceToDelay(AgoData * data, int trace[], int& traceCount)
{
	if (data && data->isDelayed) {
		traceCount = 0;
		while (data && data->ref.type != VX_TYPE_DELAY && traceCount < AGO_MAX_DEPTH_FROM_DELAY_OBJECT) {
			trace[traceCount++] = data->siblingIndex;
			data = data->parent;
		}
	}
	return (data->ref.type == VX_TYPE_DELAY) ? data : nullptr;
}

AgoData * agoGetDataFromSiblingTrace(AgoData * data, int trace[], int traceCount)
{
	for (int i = traceCount - 1; data && i >= 0; i--) {
		vx_uint32 child = (vx_uint32)trace[i];
		if (child < data->numChildren)
			data = data->children[child];
		else
			data = nullptr;
	}
	return data;
}

void agoGetDescriptionFromData(AgoContext * acontext, char * desc, AgoData * data)
{
	const char * virt = data->isVirtual ? "-virtual" : "";
	desc[0] = 0;
	if (data->ref.type == VX_TYPE_DELAY) {
		sprintf(desc + strlen(desc), "delay%s:%d,[", virt, data->u.delay.count);
		agoGetDescriptionFromData(acontext, desc + strlen(desc), data->children[0]);
		sprintf(desc + strlen(desc), "]");
	}
	else if (data->ref.type == VX_TYPE_IMAGE) {
		if (data->u.img.isROI) {
			sprintf(desc + strlen(desc), "image-roi:%s,%d,%d,%d,%d", data->u.img.roiMasterImage->name.c_str(), data->u.img.rect_roi.start_x, data->u.img.rect_roi.start_y, data->u.img.rect_roi.end_x, data->u.img.rect_roi.end_y);
		}
		else if (data->u.img.isUniform) {
			sprintf(desc + strlen(desc), "image-uniform%s:%4.4s,%d,%d", virt, FORMAT_STR(data->u.img.format), data->u.img.width, data->u.img.height);
			for (vx_size i = 0; i < data->u.img.components; i++) sprintf(desc + strlen(desc), ",%d", (unsigned int)data->u.img.uniform[i]);
		}
		else sprintf(desc + strlen(desc), "image%s:%4.4s,%d,%d", virt, FORMAT_STR(data->u.img.format), data->u.img.width, data->u.img.height);
	}
	else if (data->ref.type == VX_TYPE_PYRAMID) {
		char scale[64];
		if (data->u.pyr.scale == VX_SCALE_PYRAMID_HALF) sprintf(scale, "HALF");
		else if (data->u.pyr.scale == VX_SCALE_PYRAMID_ORB) sprintf(scale, "ORB");
		else sprintf(scale, "%g", data->u.pyr.scale);
		sprintf(desc + strlen(desc), "pyramid%s:%4.4s,%d,%d," VX_FMT_SIZE ",%s", virt, FORMAT_STR(data->u.pyr.format), data->u.pyr.width, data->u.pyr.height, data->u.pyr.levels, scale);
	}
	else if (data->ref.type == VX_TYPE_ARRAY) {
		if (data->u.arr.itemtype >= VX_TYPE_USER_STRUCT_START && data->u.arr.itemtype <= VX_TYPE_USER_STRUCT_END) {
			const char * name = agoGetUserStructName(acontext, data->u.arr.itemtype);
			if (name)
				sprintf(desc + strlen(desc), "array%s:%s," VX_FMT_SIZE "", virt, name, data->u.arr.capacity);
			else
				sprintf(desc + strlen(desc), "array%s:USER-STRUCT-" VX_FMT_SIZE "," VX_FMT_SIZE "", virt, data->u.arr.itemsize, data->u.arr.capacity);
		}
		else
			sprintf(desc + strlen(desc), "array%s:%s," VX_FMT_SIZE "", virt, agoEnum2Name(data->u.arr.itemtype), data->u.arr.capacity);
	}
	else if (data->ref.type == VX_TYPE_SCALAR) {
		if (data->u.scalar.type == VX_TYPE_ENUM) {
			const char * name = agoEnum2Name(data->u.scalar.u.e);
			if (name) 
				sprintf(desc + strlen(desc), "scalar%s:ENUM,%s", virt, name);
			else
				sprintf(desc + strlen(desc), "scalar%s:ENUM,0x%08x", virt, data->u.scalar.u.e);
		}
		else if (data->u.scalar.type == VX_TYPE_UINT32) sprintf(desc + strlen(desc), "scalar%s:UINT32,%u", virt, data->u.scalar.u.u);
		else if (data->u.scalar.type == VX_TYPE_INT32) sprintf(desc + strlen(desc), "scalar%s:INT32,%d", virt, data->u.scalar.u.i);
		else if (data->u.scalar.type == VX_TYPE_UINT16) sprintf(desc + strlen(desc), "scalar%s:UINT16,%u", virt, data->u.scalar.u.u);
		else if (data->u.scalar.type == VX_TYPE_INT16) sprintf(desc + strlen(desc), "scalar%s:INT16,%d", virt, data->u.scalar.u.i);
		else if (data->u.scalar.type == VX_TYPE_UINT8) sprintf(desc + strlen(desc), "scalar%s:UINT8,%u", virt, data->u.scalar.u.u);
		else if (data->u.scalar.type == VX_TYPE_INT8) sprintf(desc + strlen(desc), "scalar%s:INT8,%u", virt, data->u.scalar.u.i);
		else if (data->u.scalar.type == VX_TYPE_CHAR) sprintf(desc + strlen(desc), "scalar%s:CHAR,%u", virt, data->u.scalar.u.i);
		else if (data->u.scalar.type == VX_TYPE_FLOAT32) sprintf(desc + strlen(desc), "scalar%s:FLOAT32,%g", virt, data->u.scalar.u.f);
		else if (data->u.scalar.type == VX_TYPE_SIZE) sprintf(desc + strlen(desc), "scalar%s:SIZE," VX_FMT_SIZE "", virt, data->u.scalar.u.s);
		else if (data->u.scalar.type == VX_TYPE_BOOL) sprintf(desc + strlen(desc), "scalar%s:BOOL,%d", virt, data->u.scalar.u.i);
		else if (data->u.scalar.type == VX_TYPE_DF_IMAGE) sprintf(desc + strlen(desc), "scalar%s:DF_IMAGE,%4.4s", virt, (const char *)&data->u.scalar.u.df);
		else if (data->u.scalar.type == VX_TYPE_FLOAT64) sprintf(desc + strlen(desc), "scalar%s:FLOAT64,%lg", virt, data->u.scalar.u.f64);
		else if (data->u.scalar.type == VX_TYPE_INT64) sprintf(desc + strlen(desc), "scalar%s:INT64,%" PRId64, virt, data->u.scalar.u.i64);
		else if (data->u.scalar.type == VX_TYPE_UINT64) sprintf(desc + strlen(desc), "scalar%s:UINT64,%" PRIu64, virt, data->u.scalar.u.u64);
		else if (data->u.scalar.type == VX_TYPE_STRING_AMD) sprintf(desc + strlen(desc), "scalar%s:STRING,%s", virt, data->buffer ? (const char *)data->buffer : "");
		else sprintf(desc + strlen(desc), "scalar%s:UNSUPPORTED,NULL", virt);
	}
	else if (data->ref.type == VX_TYPE_DISTRIBUTION) {
		sprintf(desc + strlen(desc), "distribution%s:" VX_FMT_SIZE ",%d,%u", virt, data->u.dist.numbins, data->u.dist.offset, data->u.dist.range);
	}
	else if (data->ref.type == VX_TYPE_LUT) {
		sprintf(desc + strlen(desc), "lut%s:%s," VX_FMT_SIZE "", virt, agoEnum2Name(data->u.lut.type), data->u.lut.count);
	}
	else if (data->ref.type == VX_TYPE_THRESHOLD) {
		sprintf(desc + strlen(desc), "threshold%s:%s,%s", virt, agoEnum2Name(data->u.thr.thresh_type), agoEnum2Name(data->u.thr.data_type));
		if (data->u.thr.thresh_type == VX_THRESHOLD_TYPE_BINARY)
			sprintf(desc + strlen(desc), ":I,%d", data->u.thr.threshold_lower);
		else if (data->u.thr.thresh_type == VX_THRESHOLD_TYPE_RANGE)
			sprintf(desc + strlen(desc), ":I,%d,%d", data->u.thr.threshold_lower, data->u.thr.threshold_upper);
	}
	else if (data->ref.type == VX_TYPE_CONVOLUTION) {
		sprintf(desc + strlen(desc), "convolution%s:" VX_FMT_SIZE "," VX_FMT_SIZE "", virt, data->u.conv.columns, data->u.conv.rows);
		if (data->u.conv.shift)
			sprintf(desc + strlen(desc), ",%u", data->u.conv.shift);
	}
	else if (data->ref.type == VX_TYPE_MATRIX) {
		sprintf(desc + strlen(desc), "matrix%s:%s," VX_FMT_SIZE "," VX_FMT_SIZE "", virt, agoEnum2Name(data->u.mat.type), data->u.mat.columns, data->u.mat.rows);
	}
	else if (data->ref.type == VX_TYPE_REMAP) {
		sprintf(desc + strlen(desc), "remap%s:%u,%u,%u,%u", virt, data->u.remap.src_width, data->u.remap.src_height, data->u.remap.dst_width, data->u.remap.dst_height);
	}
	else if (data->ref.type == AGO_TYPE_MEANSTDDEV_DATA) {
		sprintf(desc + strlen(desc), "ago-meanstddev-data%s:", virt);
	}
	else if (data->ref.type == AGO_TYPE_MINMAXLOC_DATA) {
		sprintf(desc + strlen(desc), "ago-minmaxloc-data%s:", virt);
	}
	else if (data->ref.type == AGO_TYPE_CANNY_STACK) {
		sprintf(desc + strlen(desc), "ago-canny-stack%s:%u", virt, data->u.cannystack.count);
	}
	else if (data->ref.type == AGO_TYPE_SCALE_MATRIX) {
		sprintf(desc + strlen(desc), "ago-scale-matrix%s:%.12e,%.12e,%.12e,%.12e", virt, data->u.scalemat.xscale, data->u.scalemat.yscale, data->u.scalemat.xoffset, data->u.scalemat.yoffset);
	}
	else sprintf(desc + strlen(desc), "UNSUPPORTED%s:0x%08x", virt, data->ref.type);
}

int agoGetDataFromDescription(AgoContext * acontext, AgoGraph * agraph, AgoData * data, const char * desc)
{
	if (!strncmp(desc, "delay:", 6) || !strncmp(desc, "delay-virtual:", 14)) {
		if (!strncmp(desc, "delay-virtual:", 14)) {
			data->isVirtual = vx_true_e;
			desc += 14;
		}
		else desc += 6;
		// get configuration
		data->ref.type = VX_TYPE_DELAY;
		if (sscanf(desc, "%u", &data->u.delay.count) != 1) return -1;
		if (data->u.delay.count < 1) return -1;
		while (*desc && *desc != '[') desc++;
		vx_uint32 epos = (vx_uint32)strlen(desc) - 1;
		if ((*desc != '[') || (desc[epos] != ']')) return -1;
		desc++; epos--;
		char desc_child[1024];
		strncpy(desc_child, desc, epos);
		if (data->children) 
			delete [] data->children;
		data->numChildren = (vx_uint32)data->u.delay.count;
		data->children = new AgoData * [data->numChildren];
		for (vx_uint32 child = 0; child < data->numChildren; child++) {
			if ((data->children[child] = agoCreateDataFromDescription(acontext, agraph, desc_child, false)) == NULL) return -1;
			data->children[child]->parent = data;
			data->children[child]->siblingIndex = (vx_int32)child;
			if (data->children[child]->isVirtual != data->isVirtual) return -1;
			if (child == 0) {
				data->u.delay.type = data->children[child]->ref.type;
				if (data->u.delay.type == VX_TYPE_DELAY) {
					agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: delay of delay is not permitted\n");
					return -1;
				}
			}
		}
		// mark the children of delay element as part of delay object
		agoMarkChildrenAsPartOfDelay(data);
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for delay\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "image:", 6) || !strncmp(desc, "image-virtual:", 14)) {
		if (!strncmp(desc, "image-virtual:", 14)) {
			data->u.img.isVirtual = vx_true_e;
			data->isVirtual = vx_true_e;
			desc += 14;
		}
		else desc += 6;
		// get configuration
		data->ref.type = VX_TYPE_IMAGE;
		memcpy(&data->u.img.format, desc, sizeof(data->u.img.format));
		if (sscanf(desc + 5, "%d,%d", &data->u.img.width, &data->u.img.height) != 2) return -1;
		if (data->isVirtual && !data->isNotFullyConfigured && (data->u.img.format == VX_DF_IMAGE_VIRT || data->u.img.width == 0 || data->u.img.height == 0)) {
			// incomplete information needs to process this again later
			data->isNotFullyConfigured = vx_true_e;
			return 0;
		}
		if (agoGetImageComponentsAndPlanes(data->u.img.format, &data->u.img.components, &data->u.img.planes, &data->u.img.pixel_size_in_bits, &data->u.img.color_space, &data->u.img.channel_range)) return -1;
		if (data->u.img.planes > 1) {
			if (data->children) 
				delete [] data->children;
			data->numChildren = (vx_uint32)data->u.img.planes;
			data->children = new AgoData *[data->numChildren];
			for (vx_uint32 child = 0; child < data->numChildren; child++) {
				vx_df_image format;
				vx_uint32 width, height;
				if (agoGetImagePlaneFormat(data->u.img.format, data->u.img.width, data->u.img.height, child, &format, &width, &height)) return -1;
				char imgdesc[64]; sprintf(imgdesc, "image%s:%4.4s,%d,%d", data->isVirtual ? "-virtual" : "", FORMAT_STR(format), width, height);
				if ((data->children[child] = agoCreateDataFromDescription(acontext, agraph, imgdesc, false)) == NULL) return -1;
				if (agoGetImageComponentsAndPlanes(data->children[child]->u.img.format, &data->children[child]->u.img.components, &data->children[child]->u.img.planes, &data->children[child]->u.img.pixel_size_in_bits, &data->children[child]->u.img.color_space, &data->children[child]->u.img.channel_range)) return -1;
				data->children[child]->siblingIndex = (vx_int32)child;
				data->children[child]->parent = data;
				data->children[child]->u.img.x_scale_factor_is_2 = (data->children[child]->u.img.width  != data->u.img.width ) ? 1 : 0;
				data->children[child]->u.img.y_scale_factor_is_2 = (data->children[child]->u.img.height != data->u.img.height) ? 1 : 0;
			}
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for image\n");
			return -1;
		}
		// set valid region of the image to FULL AREA
		data->u.img.rect_valid.start_x = 0;
		data->u.img.rect_valid.start_y = 0;
		data->u.img.rect_valid.end_x = data->u.img.width;
		data->u.img.rect_valid.end_y = data->u.img.height;
		return 0;
	}
	else if (!strncmp(desc, "image-uniform:", 14) || !strncmp(desc, "image-uniform-virtual:", 22)) {
		if (!strncmp(desc, "image-uniform-virtual:", 22)) {
			data->u.img.isVirtual = vx_true_e;
			data->isVirtual = vx_true_e;
			desc += 22;
		}
		else desc += 14;
		// get configuration
		data->ref.type = VX_TYPE_IMAGE;
		data->u.img.isUniform = vx_true_e;
		memcpy(&data->u.img.format, desc, sizeof(data->u.img.format));
		if (sscanf(desc + 5, "%d,%d," VX_FMT_SIZE "," VX_FMT_SIZE "," VX_FMT_SIZE "," VX_FMT_SIZE "", &data->u.img.width, &data->u.img.height, &data->u.img.uniform[0], &data->u.img.uniform[1], &data->u.img.uniform[2], &data->u.img.uniform[3]) < 2) return -1;
		if (agoGetImageComponentsAndPlanes(data->u.img.format, &data->u.img.components, &data->u.img.planes, &data->u.img.pixel_size_in_bits, &data->u.img.color_space, &data->u.img.channel_range)) return -1;
		data->isInitialized = vx_true_e;
		if (data->u.img.planes > 1) {
			if (data->children) 
				delete [] data->children;
			data->numChildren = (vx_uint32)data->u.img.planes;
			data->children = new AgoData *[data->numChildren];
			for (vx_uint32 child = 0; child < data->numChildren; child++) {
				vx_df_image format;
				vx_uint32 width, height;
				if (agoGetImagePlaneFormat(data->u.img.format, data->u.img.width, data->u.img.height, child, &format, &width, &height)) return -1;
				vx_uint32 value = (vx_uint32)data->u.img.uniform[child];

				// special handling required for NV12/NV21 image formats
				if (data->u.img.format == VX_DF_IMAGE_NV21 && child == 1) value = (value << 8) | (vx_uint32)data->u.img.uniform[child + 1];
				else if (data->u.img.format == VX_DF_IMAGE_NV12 && child == 1) value = value | ((vx_uint32)data->u.img.uniform[child + 1] << 8);

				char imgdesc[64]; sprintf(imgdesc, "image-uniform%s:%4.4s,%d,%d,%d", data->isVirtual ? "-virtual" : "", FORMAT_STR(format), width, height, value);
				if ((data->children[child] = agoCreateDataFromDescription(acontext, agraph, imgdesc, false)) == NULL) return -1;
				if (agoGetImageComponentsAndPlanes(data->children[child]->u.img.format, &data->children[child]->u.img.components, &data->children[child]->u.img.planes, &data->children[child]->u.img.pixel_size_in_bits, &data->children[child]->u.img.color_space, &data->children[child]->u.img.channel_range)) return -1;
				data->children[child]->isInitialized = vx_true_e;
				data->children[child]->parent = data;
				data->children[child]->u.img.x_scale_factor_is_2 = (data->children[child]->u.img.width  != data->u.img.width ) ? 1 : 0;
				data->children[child]->u.img.y_scale_factor_is_2 = (data->children[child]->u.img.height != data->u.img.height) ? 1 : 0;
				// set min/max values as uniform value
				if (data->children[child]->u.img.format == VX_DF_IMAGE_U8 ||
					data->children[child]->u.img.format == VX_DF_IMAGE_S16 ||
					data->children[child]->u.img.format == VX_DF_IMAGE_U16 ||
					data->children[child]->u.img.format == VX_DF_IMAGE_S32 ||
					data->children[child]->u.img.format == VX_DF_IMAGE_U32 ||
					data->children[child]->u.img.format == VX_DF_IMAGE_U1_AMD)
				{
					data->children[child]->u.img.hasMinMax = vx_true_e;
					data->children[child]->u.img.minValue = (vx_int32)data->children[child]->u.img.uniform[0];
					data->children[child]->u.img.maxValue = (vx_int32)data->children[child]->u.img.uniform[0];
				}
			}
		}
		// set min/max values as uniform value
		if (data->u.img.format == VX_DF_IMAGE_U8 ||
			data->u.img.format == VX_DF_IMAGE_S16 ||
			data->u.img.format == VX_DF_IMAGE_U16 ||
			data->u.img.format == VX_DF_IMAGE_S32 ||
			data->u.img.format == VX_DF_IMAGE_U32 ||
			data->u.img.format == VX_DF_IMAGE_U1_AMD)
		{
			data->u.img.hasMinMax = vx_true_e;
			data->u.img.minValue = (vx_int32)data->u.img.uniform[0];
			data->u.img.maxValue = (vx_int32)data->u.img.uniform[0];
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for image-uniform\n");
			return -1;
		}
		// set valid region of the image to FULL AREA
		data->u.img.rect_valid.start_x = 0;
		data->u.img.rect_valid.start_y = 0;
		data->u.img.rect_valid.end_x = data->u.img.width;
		data->u.img.rect_valid.end_y = data->u.img.height;
		return 0;
	}
	else if (!strncmp(desc, "image-roi:", 10)) {
		desc += 10;
		// get configuration
		data->ref.type = VX_TYPE_IMAGE;
		data->u.img.isROI = vx_true_e;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char master_name[128];
		memcpy(master_name, desc, s - desc); master_name[s - desc] = 0;
		s++;
		if (sscanf(s, "%u,%u,%u,%u", &data->u.img.rect_roi.start_x, &data->u.img.rect_roi.start_y, &data->u.img.rect_roi.end_x, &data->u.img.rect_roi.end_y) != 4) return -1;
		vx_rectangle_t rect = data->u.img.rect_roi;
		// traverse and link ROI to top-level image
		AgoData * pdata = agoFindDataByName(acontext, agraph, master_name);
		while (pdata && pdata->ref.type == VX_TYPE_IMAGE && pdata->u.img.isROI) {
			rect.start_x += pdata->u.img.rect_roi.start_x;
			rect.start_y += pdata->u.img.rect_roi.start_y;
			rect.end_x += pdata->u.img.rect_roi.start_x;
			rect.end_y += pdata->u.img.rect_roi.start_y;
			pdata = pdata->u.img.roiMasterImage;
		}
		if (!pdata || pdata->ref.type != VX_TYPE_IMAGE) {
			agoAddLogEntry(&pdata->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: image-roi: master image is invalid: %s\n", master_name);
			return -1;
		}
		data->isVirtual = pdata->isVirtual;
		data->isInitialized = pdata->isInitialized;
		data->u.img = pdata->u.img;
		data->u.img.roiMasterImage = pdata;
		data->u.img.isROI = vx_true_e;
		data->u.img.rect_roi = rect;
		data->u.img.width = data->u.img.rect_roi.end_x - data->u.img.rect_roi.start_x;
		data->u.img.height = data->u.img.rect_roi.end_y - data->u.img.rect_roi.start_y;
		// create ROI entries for children, if image has multiple planes
		data->numChildren = pdata->numChildren;
		if (pdata->children) {
			data->children = new AgoData *[data->numChildren];
			for (vx_uint32 child = 0; child < data->numChildren; child++) {
				data->children[child] = new AgoData;
				agoResetReference(&data->children[child]->ref, data->children[child]->ref.type, acontext, data->children[child]->isVirtual ? &agraph->ref : NULL);
				data->children[child]->ref.internal_count++;
				data->children[child]->ref.type = pdata->children[child]->ref.type;
				data->children[child]->isVirtual = pdata->children[child]->isVirtual;
				data->children[child]->isInitialized = pdata->children[child]->isInitialized;
				data->children[child]->u.img = pdata->children[child]->u.img;
				data->children[child]->u.img.roiMasterImage = pdata->children[child];
				data->children[child]->u.img.isROI = vx_true_e;
				data->children[child]->u.img.rect_roi = rect;
				data->children[child]->parent = data;
				if (pdata->children[child]->u.img.width < pdata->u.img.width) {
					// this is a 2x2 decimated plane of an image: IYUV, NV12, NV21
					data->children[child]->u.img.rect_roi.start_x = data->u.img.rect_roi.start_x >> 1;
					data->children[child]->u.img.rect_roi.end_x = data->children[child]->u.img.rect_roi.start_x + ((data->u.img.width + 1) >> 1);
				}
				if (pdata->children[child]->u.img.height < pdata->u.img.height) {
					// this is a 2x2 decimated plane of an image: IYUV, NV12, NV21
					data->children[child]->u.img.rect_roi.start_y = data->u.img.rect_roi.start_y >> 1;
					data->children[child]->u.img.rect_roi.end_y = data->children[child]->u.img.rect_roi.start_y + ((data->u.img.height + 1) >> 1);
				}
				data->children[child]->u.img.width = data->children[child]->u.img.rect_roi.end_x - data->children[child]->u.img.rect_roi.start_x;
				data->children[child]->u.img.height = data->children[child]->u.img.rect_roi.end_y - data->children[child]->u.img.rect_roi.start_y;
				data->children[child]->u.img.x_scale_factor_is_2 = (data->children[child]->u.img.width  != data->u.img.width ) ? 1 : 0;
				data->children[child]->u.img.y_scale_factor_is_2 = (data->children[child]->u.img.height != data->u.img.height) ? 1 : 0;
			}
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for image-roi\n");
			return -1;
		}
		// set valid region of the image to FULL AREA
		data->u.img.rect_valid.start_x = 0;
		data->u.img.rect_valid.start_y = 0;
		data->u.img.rect_valid.end_x = data->u.img.width;
		data->u.img.rect_valid.end_y = data->u.img.height;
		return 0;
	}
	else if (!strncmp(desc, "pyramid:", 8) || !strncmp(desc, "pyramid-virtual:", 8 + 8)) {
		data->isVirtual = !strncmp(desc, "pyramid-virtual:", 8 + 8) ? vx_true_e : vx_false_e;
		desc += 8 + (data->isVirtual ? 8 : 0);
		// get configuration
		data->ref.type = VX_TYPE_PYRAMID;
		memcpy(&data->u.pyr.format, desc, sizeof(data->u.pyr.format));
		char scale[64] = "";
		if (sscanf(desc + 5, "%d,%d," VX_FMT_SIZE ",%s", &data->u.pyr.width, &data->u.pyr.height, &data->u.pyr.levels, scale) != 4) return -1;
		if (!strncmp(scale, "HALF", 4)) data->u.pyr.scale = VX_SCALE_PYRAMID_HALF;
		else if (!strncmp(scale, "ORB", 3)) data->u.pyr.scale = VX_SCALE_PYRAMID_ORB;
		else data->u.pyr.scale = (vx_float32)atof(scale);
		if (data->isVirtual && !data->isNotFullyConfigured && (data->u.pyr.format == VX_DF_IMAGE_VIRT || data->u.pyr.width == 0 || data->u.pyr.height == 0)) {
			// incomplete information needs to process this again later
			data->isNotFullyConfigured = vx_true_e;
			return 0;
		}
		if (data->children) 
			delete [] data->children;
		data->numChildren = (vx_uint32)data->u.pyr.levels;
		data->children = new AgoData *[data->numChildren];
		for (vx_uint32 level = 0, width = data->u.pyr.width, height = data->u.pyr.height; level < data->u.pyr.levels; level++) {
			char imgdesc[64];
			sprintf(imgdesc, "image%s:%4.4s,%d,%d", data->isVirtual ? "-virtual" : "", FORMAT_STR(data->u.pyr.format), width, height);
			if ((data->children[level] = agoCreateDataFromDescription(acontext, agraph, imgdesc, false)) == NULL) return -1;
			if (agoGetImageComponentsAndPlanes(data->u.pyr.format, &data->children[level]->u.img.components, &data->children[level]->u.img.planes, &data->children[level]->u.img.pixel_size_in_bits, &data->children[level]->u.img.color_space, &data->children[level]->u.img.channel_range)) return -1;
			data->children[level]->siblingIndex = (vx_int32)level;
			data->children[level]->parent = data;
			if (data->u.pyr.scale == VX_SCALE_PYRAMID_ORB) {
				float orb_scale_factor[4] = {
					VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_HALF
				};
				width = (vx_uint32)ceilf(orb_scale_factor[level & 3] * data->children[level & ~3]->u.img.width);
				height = (vx_uint32)ceilf(orb_scale_factor[level & 3] * data->children[level & ~3]->u.img.height);
			}
			else {
				width = (vx_uint32)ceilf(data->u.pyr.scale * width);
				height = (vx_uint32)ceilf(data->u.pyr.scale * height);
			}
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for pyramid\n");
			return -1;
		}
		// set valid region of the pyramid to FULL AREA
		data->u.pyr.rect_valid.start_x = 0;
		data->u.pyr.rect_valid.start_y = 0;
		data->u.pyr.rect_valid.end_x = data->u.pyr.width;
		data->u.pyr.rect_valid.end_y = data->u.pyr.height;
		return 0;
	}
	else if (!strncmp(desc, "array:", 6) || !strncmp(desc, "array-virtual:", 6 + 8)) {
		if (!strncmp(desc, "array-virtual:", 6 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 6;
		// get configuration
		data->ref.type = VX_TYPE_ARRAY;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char data_type[64];
		memcpy(data_type, desc, s - desc); data_type[s - desc] = 0;
		(void)sscanf(++s, "" VX_FMT_SIZE "", &data->u.arr.capacity);
		data->u.arr.itemtype = agoName2Enum(data_type);
		if (!data->u.arr.itemtype) data->u.arr.itemtype = atoi(data_type);
		if (data->isVirtual && !data->isNotFullyConfigured && (!strcmp(data_type, "0") || !data->u.arr.capacity)) {
			// incomplete information needs to process this again later
			data->isNotFullyConfigured = vx_true_e;
			return 0;
		}
		data->u.arr.itemsize = agoType2Size(acontext, data->u.arr.itemtype);
		if (!data->u.arr.itemsize) {
			vx_enum id = agoGetUserStructType(acontext, data_type);
			if (!id) {
				agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: invalid data type in array: %s\n", data_type);
				return -1;
			}
			data->u.arr.itemtype = id;
		}
		// sanity check and update
		data->ref.context = acontext; // array sanity check requires access to context
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for array\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "distribution:", 13) || !strncmp(desc, "distribution-virtual:", 13 + 8)) {
		if (!strncmp(desc, "distribution-virtual:", 13 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 13;
		// get configuration
		data->ref.type = VX_TYPE_DISTRIBUTION;
		if (sscanf(desc, "" VX_FMT_SIZE ",%d,%u", &data->u.dist.numbins, &data->u.dist.offset, &data->u.dist.range) != 3) 
			return -1;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for distribution\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "lut:", 4) || !strncmp(desc, "lut-virtual:", 4 + 8)) {
		if (!strncmp(desc, "lut-virtual:", 4 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 4;
		// get configuration
		data->ref.type = VX_TYPE_LUT;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char data_type[64];
		memcpy(data_type, desc, s - desc); data_type[s - desc] = 0;
		data->u.lut.type = agoName2Enum(data_type);
		if (!data->u.lut.type) 
			return -1;
		if (sscanf(++s, "" VX_FMT_SIZE "", &data->u.lut.count) != 1) return -1;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for lut\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "threshold:", 10) || !strncmp(desc, "threshold-virtual:", 10 + 8)) {
		if (!strncmp(desc, "threshold-virtual:", 10 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 10;
		// get configuration
		data->ref.type = VX_TYPE_THRESHOLD;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char thresh_type[64], data_type[64];
		memcpy(thresh_type, desc, s - desc); thresh_type[s - desc] = 0;
		strcpy(data_type, s + 1);
		for (int i = 0; i < 64 && data_type[i]; i++) if (data_type[i] == ':' || data_type[i] == ',') { data_type[i] = 0; s += i + 2; break; }
		data->u.thr.thresh_type = agoName2Enum(thresh_type);
		data->u.thr.data_type = agoName2Enum(data_type);
		if (!data->u.thr.thresh_type || !data->u.thr.data_type) return -1;
		if (data->u.thr.data_type != VX_TYPE_UINT8 && data->u.thr.data_type != VX_TYPE_UINT16 && data->u.thr.data_type != VX_TYPE_INT16) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: invalid threshold data_type %s\n", data_type);
			return -1;
		}
		if (data->u.thr.thresh_type == VX_THRESHOLD_TYPE_BINARY) {
			if (sscanf(s, "%d", &data->u.thr.threshold_lower) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.thr.thresh_type == VX_THRESHOLD_TYPE_RANGE) {
			if (sscanf(s, "%d,%d", &data->u.thr.threshold_lower, &data->u.thr.threshold_upper) == 2)
				data->isInitialized = vx_true_e;
		}
		else {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: invalid threshold thresh_type %s\n", thresh_type);
			return -1;
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for threshold\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "convolution:", 12) || !strncmp(desc, "convolution-virtual:", 12 + 8)) {
		if (!strncmp(desc, "convolution-virtual:", 12 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 12;
		// get configuration
		data->ref.type = VX_TYPE_CONVOLUTION;
		vx_uint32 scale = 1;
		if (sscanf(desc, "" VX_FMT_SIZE "," VX_FMT_SIZE ",%u", &data->u.conv.columns, &data->u.conv.rows, &scale) < 2)
			return -1;
		vx_uint32 shift = 0;
		for (; shift < 32; shift++) {
			if (scale == (1u << shift))
				break;
		}
		data->u.conv.shift = shift;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for convolution\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "matrix:", 7) || !strncmp(desc, "matrix-virtual:", 7 + 8)) {
		if (!strncmp(desc, "matrix-virtual:", 7 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 7;
		// get configuration
		data->ref.type = VX_TYPE_MATRIX;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char data_type[64];
		memcpy(data_type, desc, s - desc); data_type[s - desc] = 0;
		data->u.mat.type = agoName2Enum(data_type);
		if (data->u.mat.type != VX_TYPE_INT32 && data->u.mat.type != VX_TYPE_FLOAT32) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: invalid matrix type %s\n", data_type);
			return -1;
		}
		if (sscanf(++s, "" VX_FMT_SIZE "," VX_FMT_SIZE "", &data->u.mat.columns, &data->u.mat.rows) != 2)
			return -1;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for matrix\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "remap:", 6) || !strncmp(desc, "remap-virtual:", 6 + 8)) {
		if (!strncmp(desc, "remap-virtual:", 6 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 6;
		// get configuration
		data->ref.type = VX_TYPE_REMAP;
		if (sscanf(desc, "%u,%u,%u,%u", &data->u.remap.src_width, &data->u.remap.src_height, &data->u.remap.dst_width, &data->u.remap.dst_height) != 4) 
			return -1;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for remap\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "scalar:", 7) || !strncmp(desc, "scalar-virtual:", 7 + 8)) {
		if (!strncmp(desc, "scalar-virtual:", 7 + 8)) {
			desc += 8;
			data->isVirtual = vx_true_e;
		}
		desc += 7;
		// get configuration
		data->ref.type = VX_TYPE_SCALAR;
		const char *s = strstr(desc, ","); if (!s) return -1;
		char data_type[64];
		memcpy(data_type, desc, s - desc); data_type[s - desc] = 0;
		s++;
		data->u.scalar.type = agoName2Enum(data_type);
		data->u.scalar.u.u64 = 0;
		if (data->u.scalar.type == VX_TYPE_UINT32) {
			data->u.scalar.itemsize = sizeof(vx_uint32);
			if (sscanf(s, "%u", &data->u.scalar.u.u) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_INT32) {
			data->u.scalar.itemsize = sizeof(vx_int32);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_UINT16) {
			data->u.scalar.itemsize = sizeof(vx_uint16);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_INT16) {
			data->u.scalar.itemsize = sizeof(vx_int16);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_UINT8) {
			data->u.scalar.itemsize = sizeof(vx_uint8);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_INT8) {
			data->u.scalar.itemsize = sizeof(vx_int8);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_CHAR) {
			data->u.scalar.itemsize = sizeof(vx_char);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_FLOAT32) {
			data->u.scalar.itemsize = sizeof(vx_float32);
			if (sscanf(s, "%g", &data->u.scalar.u.f) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_BOOL) {
			data->u.scalar.itemsize = sizeof(vx_bool);
			if (sscanf(s, "%d", &data->u.scalar.u.i) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_SIZE) {
			data->u.scalar.itemsize = sizeof(vx_size);
			if (sscanf(s, "" VX_FMT_SIZE "", &data->u.scalar.u.s))
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_ENUM) {
			data->u.scalar.itemsize = sizeof(vx_enum);
			data->u.scalar.u.e = agoName2Enum(s);
			if (!data->u.scalar.u.e) {
				if (sscanf(s, "%i", &data->u.scalar.u.e) != 1) {
					agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription(*,%s) invalid enum value\n", desc);
					return -1;
				}
				data->isInitialized = vx_true_e;
			}
		}
		else if (data->u.scalar.type == VX_TYPE_DF_IMAGE) {
			data->u.scalar.itemsize = sizeof(vx_df_image);
			if (strlen(s) >= 4) {
				data->u.scalar.u.df = VX_DF_IMAGE(s[0], s[1], s[2], s[3]);
				data->isInitialized = vx_true_e;
			}
		}
		else if (data->u.scalar.type == VX_TYPE_FLOAT64) {
			data->u.scalar.itemsize = sizeof(vx_float64);
			if (sscanf(s, "%lg", &data->u.scalar.u.f64) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_INT64) {
			data->u.scalar.itemsize = sizeof(vx_int64);
			if (sscanf(s, "%" PRId64, &data->u.scalar.u.i64) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_UINT64) {
			data->u.scalar.itemsize = sizeof(vx_uint64);
			if (sscanf(s, "%" PRIu64, &data->u.scalar.u.u64) == 1)
				data->isInitialized = vx_true_e;
		}
		else if (data->u.scalar.type == VX_TYPE_STRING_AMD) {
			data->u.scalar.itemsize = sizeof(vx_char *);
			data->size = VX_MAX_STRING_BUFFER_SIZE_AMD;
			data->buffer_allocated = data->buffer = (vx_uint8 *)agoAllocMemory(data->size);
			if (!data->buffer_allocated) {
				agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: memory allocation (%d) failed for %s\n", (int)data->size, data_type);
				return -1;
			}
			strncpy((char *)data->buffer, s, VX_MAX_STRING_BUFFER_SIZE_AMD);
			data->buffer[VX_MAX_STRING_BUFFER_SIZE_AMD - 1] = 0; // NUL terminate string in case of overflow
			data->isInitialized = vx_true_e;
		}
		else {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: invalid scalar type %s\n", data_type);
			return -1;
		}
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for scalar\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "ago-meanstddev-data:", 20) || !strncmp(desc, "ago-meanstddev-data-virtual:", 20 + 8)) {
		if (!strncmp(desc, "ago-meanstddev-data-virtual:", 20 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 20;
		// get configuration
		data->ref.type = AGO_TYPE_MEANSTDDEV_DATA;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for ago-meanstddev-data\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "ago-minmaxloc-data:", 19) || !strncmp(desc, "ago-minmaxloc-data-virtual:", 19 + 8)) {
		if (!strncmp(desc, "ago-minmaxloc-data-virtual:", 19 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 19;
		// get configuration
		data->ref.type = AGO_TYPE_MINMAXLOC_DATA;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for ago-minmaxloc-data\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "ago-canny-stack:", 16) || !strncmp(desc, "ago-canny-stack-virtual:", 16 + 8)) {
		if (!strncmp(desc, "ago-canny-stack-virtual:", 16 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 16;
		// get configuration
		data->ref.type = AGO_TYPE_CANNY_STACK;
		if (sscanf(desc, "%u", &data->u.cannystack.count) != 1) return -1;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for ago-canny-stack\n");
			return -1;
		}
		return 0;
	}
	else if (!strncmp(desc, "ago-scale-matrix:", 17) || !strncmp(desc, "ago-scale-matrix-virtual:", 17 + 8)) {
		if (!strncmp(desc, "ago-scale-matrix-virtual:", 17 + 8)) {
			data->isVirtual = vx_true_e;
			desc += 8;
		}
		desc += 17;
		// get configuration
		data->ref.type = AGO_TYPE_SCALE_MATRIX;
		if (sscanf(desc, "%g,%g,%g,%g", &data->u.scalemat.xscale, &data->u.scalemat.yscale, &data->u.scalemat.xoffset, &data->u.scalemat.yoffset) != 4) 
			return -1;
		data->isInitialized = vx_true_e;
		// sanity check and update
		if (agoDataSanityCheckAndUpdate(data)) {
			agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoGetDataFromDescription: agoDataSanityCheckAndUpdate failed for ago-scale-matrix\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

AgoData * agoCreateDataFromDescription(AgoContext * acontext, AgoGraph * agraph, const char * desc, bool isForExternalUse)
{
	AgoData * data = new AgoData;
	int status = agoGetDataFromDescription(acontext, agraph, data, desc);
	if (status < 0) {
		agoAddLogEntry(&acontext->ref, VX_FAILURE, "ERROR: agoCreateDataFromDescription: agoGetDataFromDescription(%s) failed\n", desc);
		delete data;
		return NULL;
	}
	agoResetReference(&data->ref, data->ref.type, acontext, data->isVirtual ? &agraph->ref : NULL);
	if (isForExternalUse) {
		data->ref.external_count = 1;
	}
	else {
		data->ref.internal_count = 1;
	}
	return data;
}

void agoGenerateDataName(AgoContext * acontext, const char * postfix, std::string& name_)
{
	char name[1024];
	sprintf(name, "AUTOX!%04d!%s", acontext->dataGenerationCount++, postfix);
	name_ = name;
}

void agoGenerateVirtualDataName(AgoGraph * agraph, const char * postfix, std::string& name_)
{
	char name[1024];
	sprintf(name, "AUTO!%04d!%s", agraph->virtualDataGenerationCount++, postfix);
	name_ = name;
}

int agoGetImageComponentsAndPlanes(vx_df_image format, vx_size * pComponents, vx_size * pPlanes, vx_size * pPixelSizeInBits, vx_color_space_e * pColorSpace, vx_channel_range_e * pChannelRange)
{
	if (format == VX_DF_IMAGE_RGBX) *pComponents = 4, *pPlanes = 1, *pPixelSizeInBits = 4 * 8, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_RGB) *pComponents = 3, *pPlanes = 1, *pPixelSizeInBits = 3 * 8, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_NV12) *pComponents = 3, *pPlanes = 2, *pPixelSizeInBits = 0, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_NV21) *pComponents = 3, *pPlanes = 2, *pPixelSizeInBits = 0, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_UYVY) *pComponents = 3, *pPlanes = 1, *pPixelSizeInBits = 2 * 8, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_YUYV) *pComponents = 3, *pPlanes = 1, *pPixelSizeInBits = 2 * 8, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_IYUV) *pComponents = 3, *pPlanes = 3, *pPixelSizeInBits = 0, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_YUV4) *pComponents = 3, *pPlanes = 3, *pPixelSizeInBits = 0, *pColorSpace = VX_COLOR_SPACE_DEFAULT, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_U8) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 8, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_U16) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 16, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_S16) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 16, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_U32) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 32, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_S32) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 32, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_U1_AMD) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 1, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_F32x3_AMD) *pComponents = 3, *pPlanes = 1, *pPixelSizeInBits = 3*32, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_F32_AMD) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 32, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else if (format == VX_DF_IMAGE_F64_AMD) *pComponents = 1, *pPlanes = 1, *pPixelSizeInBits = 64, *pColorSpace = VX_COLOR_SPACE_NONE, *pChannelRange = VX_CHANNEL_RANGE_FULL;
	else return -1;
	return 0;
}

int agoGetImagePlaneFormat(vx_df_image format, vx_uint32 width, vx_uint32 height, vx_uint32 plane, vx_df_image *pFormat, vx_uint32 * pWidth, vx_uint32 * pHeight)
{
	if (format == VX_DF_IMAGE_YUV4) {
		if (plane < 3) {
			*pFormat = VX_DF_IMAGE_U8;
			*pWidth = width;
			*pHeight = height;
			return 0;
		}
	}
	else if (format == VX_DF_IMAGE_IYUV) {
		if (plane == 0) {
			*pFormat = VX_DF_IMAGE_U8;
			*pWidth = width;
			*pHeight = height;
			return 0;
		}
		else if (plane < 3) {
			*pFormat = VX_DF_IMAGE_U8;
			*pWidth = (width + 1) >> 1;
			*pHeight = (height + 1) >> 1;
			return 0;
		}
	}
	else if (format == VX_DF_IMAGE_NV12 || format == VX_DF_IMAGE_NV21) {
		if (plane == 0) {
			*pFormat = VX_DF_IMAGE_U8;
			*pWidth = width;
			*pHeight = height;
			return 0;
		}
		else if (plane == 1) {
			*pFormat = VX_DF_IMAGE_U16;
			*pWidth = (width + 1) >> 1;
			*pHeight = (height + 1) >> 1;
			return 0;
		}
	}
	else {
		if (plane == 0) {
			*pFormat = format;
			*pWidth = width;
			*pHeight = height;
			return 0;
		}
	}
	return -1;
}

void agoGetDataName(vx_char * name, AgoData * data)
{
	name[0] = 0;
	for (AgoData * pdata = data; pdata; pdata = pdata->parent) {
		char tmp[1024]; strcpy(tmp, name);
		if (pdata->parent) {
			sprintf(name, "[%d]%s", (pdata->parent->ref.type == VX_TYPE_DELAY) ? -pdata->siblingIndex : pdata->siblingIndex, tmp);
		}
		else if (pdata->name.length()) {
			sprintf(name, "%s%s", pdata->name.c_str(), tmp);
		}
		else {
			name[0] = 0;
			break;
		}
	}
}

vx_enum agoAddUserStruct(AgoContext * acontext, vx_size size, vx_char * name)
{
	CAgoLock lock(acontext->cs);
	if (name && agoGetUserStructSize(acontext, name) > 0) {
		agoAddLogEntry(&acontext->ref, VX_FAILURE, "ERROR: agoAddUserStruct(*," VX_FMT_SIZE ",%s): already exists\n", size, name);
		return VX_TYPE_INVALID;
	}
	if (acontext->nextUserStructId >= (VX_TYPE_USER_STRUCT_START + 256)) {
		agoAddLogEntry(&acontext->ref, VX_FAILURE, "ERROR: agoAddUserStruct(*," VX_FMT_SIZE ",%s): number of user-structures exceeded MAX\n", size, name ? name : "*");
		return VX_TYPE_INVALID;
	}
	AgoUserStruct aus;
	aus.id = acontext->nextUserStructId++;
	aus.size = size;
	if(name) aus.name = name;
	else agoGenerateDataName(acontext, "UserStruct", aus.name);
	acontext->userStructList.push_back(aus);
	return aus.id;
}

vx_size agoGetUserStructSize(AgoContext * acontext, vx_char * name)
{
	for (auto it = acontext->userStructList.begin(); it != acontext->userStructList.end(); it++) {
		if (!strcmp(it->name.c_str(), name)) {
			return it->size;
		}
	}
	return 0;
}

vx_size agoGetUserStructSize(AgoContext * acontext, vx_enum id)
{
	for (auto it = acontext->userStructList.begin(); it != acontext->userStructList.end(); it++) {
		if (it->id == id) {
			return it->size;
		}
	}
	return 0;
}

vx_enum agoGetUserStructType(AgoContext * acontext, vx_char * name)
{
	for (auto it = acontext->userStructList.begin(); it != acontext->userStructList.end(); it++) {
		if (!strcmp(it->name.c_str(), name)) {
			return it->id;
		}
	}
	return 0;
}

const char * agoGetUserStructName(AgoContext * acontext, vx_enum id)
{
	for (auto it = acontext->userStructList.begin(); it != acontext->userStructList.end(); it++) {
		if (it->id == id) {
			return it->name.c_str();
		}
	}
	return nullptr;
}

bool agoIsValidParameter(vx_parameter parameter)
{
	bool ret = false;
	if (parameter && parameter->ref.type == VX_TYPE_PARAMETER && parameter->scope && parameter->scope->magic == AGO_MAGIC_VALID &&
		((parameter->scope->type == VX_TYPE_NODE) || (parameter->scope->type == VX_TYPE_KERNEL) || (parameter->scope->type == VX_TYPE_GRAPH)))
	{
		ret = true;
	}
	return ret;
}

bool agoIsValidReference(vx_reference ref)
{
	bool ret = false;
	if ((ref != NULL) && (ref->magic == AGO_MAGIC_VALID) && ((ref->external_count + ref->internal_count) > 0)) {
		ret = true;
	}
	return ret;
}

bool agoIsValidContext(vx_context context)
{
	bool ret = false;
	if (agoIsValidReference((vx_reference) context) && (context->ref.type == VX_TYPE_CONTEXT)) {
		ret = true; /* this is the top level context */
	}
	return ret;
}

bool agoIsValidGraph(vx_graph graph)
{
	bool ret = false;
	if (agoIsValidReference((vx_reference) graph) && (graph->ref.type == VX_TYPE_GRAPH)) {
		ret = true;
	}
	return ret;
}

bool agoIsValidKernel(vx_kernel kernel)
{
	bool ret = false;
	if (agoIsValidReference((vx_reference) kernel) && (kernel->ref.type == VX_TYPE_KERNEL)) {
		ret = true;
	}
	return ret;
}

bool agoIsValidNode(vx_node node)
{
	bool ret = false;
	if (agoIsValidReference((vx_reference) node) && (node->ref.type == VX_TYPE_NODE)) {
		ret = true;
	}
	return ret;
}

bool agoIsValidData(AgoData * data, vx_enum type)
{
	bool ret = false;
	if (agoIsValidReference((vx_reference) data) && (data->ref.type == type)) {
		ret = true;
	}
	return ret;
}

int agoDataSanityCheckAndUpdate(AgoData * data)
{
	if (data->ref.type == VX_TYPE_DELAY) {
		// make sure number of children is +ve integer and consistent number of children exist
		if (data->u.delay.count < 1 || !data->children || data->numChildren != data->u.delay.count)
			return -1;
		// do sanity check and update on each children
		for (vx_uint32 child = 0; child < data->numChildren; child++) {
			if (!data->children[child] || agoDataSanityCheckAndUpdate(data->children[child]))
				return -1;
			// make sure delay type matches with it's children
			if (data->u.delay.type != data->children[child]->ref.type)
				return -1;
		}
		// initialize/update other attributes (if needed)
		data->u.delay.age = 0;
	}
	else if (data->ref.type == VX_TYPE_PYRAMID) {
		// make sure number of children is +ve integer and consistent number of children exist
		if (data->u.pyr.levels < 1 || !data->children || data->numChildren != data->u.pyr.levels)
			return -1;
		// restrict the range of scale factors to 1/8 to 8
		if (data->u.pyr.scale < 0.125f || data->u.pyr.scale > 8.0f)
			return -1;
		// do sanity check and update on each children
		for (vx_uint32 level = 0, width = data->u.pyr.width, height = data->u.pyr.height; level < data->u.pyr.levels; level++) {
			// make sure children are valid images of same type
			if (!data->children[level] || data->children[level]->ref.type != VX_TYPE_IMAGE || data->u.pyr.format != data->children[level]->u.img.format)
				return -1;
			// set width and height of children
			data->children[level]->u.img.width = width;
			data->children[level]->u.img.height = height;
			if (data->u.pyr.scale == VX_SCALE_PYRAMID_ORB) {
				float orb_scale_factor[4] = {
					VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB * VX_SCALE_PYRAMID_ORB,
					VX_SCALE_PYRAMID_HALF
				};
				width = (vx_uint32)ceilf(orb_scale_factor[level & 3] * data->children[level & ~3]->u.img.width);
				height = (vx_uint32)ceilf(orb_scale_factor[level & 3] * data->children[level & ~3]->u.img.height);
			}
			else {
				width = (vx_uint32)ceilf(data->u.pyr.scale * width);
				height = (vx_uint32)ceilf(data->u.pyr.scale * height);
			}
			// sanity check and update the images
			if (agoDataSanityCheckAndUpdate(data->children[level]))
				return -1;
		}
		data->size = sizeof(ago_pyramid_u8_t) * data->u.pyr.levels;
	}
	else if (data->ref.type == VX_TYPE_IMAGE) {
		if (data->children) {
			for (vx_uint32 child = 0; child < data->numChildren; child++) {
				if (!data->children[child] || agoDataSanityCheckAndUpdate(data->children[child]))
					return -1;
				data->children[child]->u.img.x_scale_factor_is_2 = (data->children[child]->u.img.width != data->u.img.width) ? 1 : 0;
				data->children[child]->u.img.y_scale_factor_is_2 = (data->children[child]->u.img.height != data->u.img.height) ? 1 : 0;
			}
		}
		else if (data->u.img.isROI) {
			// re-compute image parameters to deal with parameter changes
			agoGetImageComponentsAndPlanes(data->u.img.format, &data->u.img.components, &data->u.img.planes, &data->u.img.pixel_size_in_bits, &data->u.img.color_space, &data->u.img.channel_range);
			// get buffer stride and compute buffer start address
			data->u.img.stride_in_bytes = data->u.img.roiMasterImage->u.img.stride_in_bytes;
			if ((data->u.img.rect_roi.start_x * data->u.img.pixel_size_in_bits) & 7) {
				agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: detected U1 ROI that doesn't start on 8-bit boundary: %s at (%d,%d)\n", data->name.length() ? data->name.c_str() : "<?>", data->u.img.rect_roi.start_x, data->u.img.rect_roi.start_y);
				return -1;
			}
		}
		else {
			// re-compute image parameters to deal with parameter changes
			// NOTE: image buffer stride needs to be multiple of 16 bytes to support CPU/GPU optimizations
			// NOTE: image buffer height needs to be mutliple of 16 to support OpenCL workgroup height=16
			agoGetImageComponentsAndPlanes(data->u.img.format, &data->u.img.components, &data->u.img.planes, &data->u.img.pixel_size_in_bits, &data->u.img.color_space, &data->u.img.channel_range);
			if (data->u.img.isUniform) {
				// calculate other attributes and buffer size:
				//   - make sure that the stride is multiple of 16 bytes
				data->u.img.stride_in_bytes = ALIGN16((data->u.img.width * data->u.img.pixel_size_in_bits + 7) >> 3);
				data->size = ALIGN16(data->u.img.height) * data->u.img.stride_in_bytes;
				if (!data->size) 
					return -1;
				// set min/max values as uniform value
				if (data->u.img.format == VX_DF_IMAGE_U8 ||
					data->u.img.format == VX_DF_IMAGE_S16 ||
					data->u.img.format == VX_DF_IMAGE_U16 ||
					data->u.img.format == VX_DF_IMAGE_S32 ||
					data->u.img.format == VX_DF_IMAGE_U32 ||
					data->u.img.format == VX_DF_IMAGE_U1_AMD)
				{
					data->u.img.hasMinMax = vx_true_e;
					data->u.img.minValue = (vx_int32)data->u.img.uniform[0];
					data->u.img.maxValue = (vx_int32)data->u.img.uniform[0];
				}
			}
			else {
				// calculate other attributes and buffer size:
				//   - make sure that the stride is multiple of 16 bytes
				data->u.img.stride_in_bytes = (((data->u.img.width * data->u.img.pixel_size_in_bits + 7) >> 3) + 15) & ~15;
				data->size = ALIGN16(data->u.img.height) * data->u.img.stride_in_bytes;
				if (!data->size) 
					return -1;
			}
		}
	}
	else if (data->ref.type == VX_TYPE_ARRAY) {
		// calculate other attributes and buffer size
		data->u.arr.itemsize = agoType2Size(data->ref.context, data->u.arr.itemtype);
		if (!data->u.arr.itemsize) {
			vx_size size = agoGetUserStructSize(data->ref.context, data->u.arr.itemtype);
			if(!size)
				return -1;
			data->u.arr.itemsize = size;
		}
		data->size = data->u.arr.itemsize * data->u.arr.capacity;
		if (!data->size) 
			return -1;
		data->u.arr.numitems = 0;
	}
	else if (data->ref.type == VX_TYPE_DISTRIBUTION) {
		// calculate other attributes and buffer size
		data->size = data->u.dist.numbins * sizeof(vx_uint32);
		if (!data->size) 
			return -1;
		data->u.dist.window = (vx_uint32)((data->u.dist.range + data->u.dist.numbins - 1) / data->u.dist.numbins);
	}
	else if (data->ref.type == VX_TYPE_LUT) {
		// calculate other attributes and buffer size
		if (data->u.lut.type != VX_TYPE_UINT8 || data->u.lut.count != 256) return -1;
		data->size = sizeof(vx_uint8) * 256;
		if (!data->size) 
			return -1;
	}
	else if (data->ref.type == VX_TYPE_THRESHOLD) {
		// calculate other attributes and buffer size
		data->u.thr.false_value = 0;
		if (data->u.thr.data_type == VX_TYPE_UINT8) data->u.thr.true_value = 0xff;
		else if (data->u.thr.data_type == VX_TYPE_UINT16) data->u.thr.true_value = 0xffff;
		else if (data->u.thr.data_type == VX_TYPE_INT16) data->u.thr.true_value = 0x7fff;
		else
			return -1;
	}
	else if (data->ref.type == VX_TYPE_CONVOLUTION) {
		// check validity of shift
		if (data->u.conv.shift >= 32)
			return -1;
		// calculate other attributes and buffer size
		data->size = data->u.conv.columns * data->u.conv.rows * sizeof(vx_int16);
		if (!data->size) 
			return -1;
	}
	else if (data->ref.type == VX_TYPE_MATRIX) {
		// calculate other attributes and buffer size
		if (data->u.mat.type == VX_TYPE_INT32) 
			data->u.mat.itemsize = sizeof(vx_int32);
		else if (data->u.mat.type == VX_TYPE_FLOAT32) 
			data->u.mat.itemsize = sizeof(vx_float32);
		else
			return -1;
		data->size = data->u.mat.columns * data->u.mat.rows * data->u.mat.itemsize;
		if (!data->size) 
			return -1;
	}
	else if (data->ref.type == VX_TYPE_REMAP) {
		// calculate remap_fractional_bits
		if (data->u.remap.src_width >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 3))) || data->u.remap.src_height >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 3))))
			data->u.remap.remap_fractional_bits = AGO_REMAP_FRACTIONAL_BITS - 3;
		else if (data->u.remap.src_width >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 2))) || data->u.remap.src_height >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 2))))
			data->u.remap.remap_fractional_bits = AGO_REMAP_FRACTIONAL_BITS - 2;
		else if (data->u.remap.src_width >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 1))) || data->u.remap.src_height >= (1 << (15 - (AGO_REMAP_FRACTIONAL_BITS - 1))))
			data->u.remap.remap_fractional_bits = AGO_REMAP_FRACTIONAL_BITS - 1;
		else
			data->u.remap.remap_fractional_bits = AGO_REMAP_FRACTIONAL_BITS;
		// calculate other attributes and buffer size
		data->size = data->u.remap.dst_width * data->u.remap.dst_height * sizeof(ago_coord2d_ushort_t);
		if (!data->size) 
			return -1;
	}
	else if (data->ref.type == VX_TYPE_SCALAR) {
		// nothing to do
	}
	else if (data->ref.type == VX_TYPE_STRING_AMD) {
		// nothing to do
	}
	else if (data->ref.type == AGO_TYPE_MEANSTDDEV_DATA) {
		// calculate other attributes and buffer size
		data->size = sizeof(ago_meanstddev_data_t);
	}
	else if (data->ref.type == AGO_TYPE_MINMAXLOC_DATA) {
		// calculate other attributes and buffer size
		data->size = sizeof(ago_minmaxloc_data_t);
	}
	else if (data->ref.type == AGO_TYPE_CANNY_STACK) {
		// calculate other attributes and buffer size
		data->u.cannystack.stackTop = 0;
		data->size = sizeof(ago_coord2d_ushort_t) * data->u.cannystack.count;
		if (!data->size) 
			return -1;
	}
	else if (data->ref.type == AGO_TYPE_SCALE_MATRIX) {
		// nothing to do
	}
	else return -1;
	return 0;
}

int agoAllocData(AgoData * data)
{
	if (data->buffer) {
		// already allocated: nothing to do
		return 0;
	}
	else if (agoDataSanityCheckAndUpdate(data)) {
		// can't proceed further
		return -1;
	}

	if (data->ref.type == VX_TYPE_DELAY) {
		for (vx_uint32 child = 0; child < data->numChildren; child++) {
			if (data->children[child]) {
				if (agoAllocData(data->children[child])) {
					return -1;
				}
			}
		}
	}
	else if (data->ref.type == VX_TYPE_PYRAMID) {
		for (vx_uint32 child = 0; child < data->numChildren; child++) {
			if (data->children[child]) {
				if (agoAllocData(data->children[child])) {
					return -1;
				}
			}
		}
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
		// initialize pyramid image information
		ago_pyramid_u8_t * pyrInfo = (ago_pyramid_u8_t *) data->buffer;
		for (vx_uint32 child = 0; child < data->numChildren; child++) {
			if (data->children[child]) {
				pyrInfo[child].width = data->children[child]->u.img.width;
				pyrInfo[child].height = data->children[child]->u.img.height;
				pyrInfo[child].strideInBytes = data->children[child]->u.img.stride_in_bytes;
				pyrInfo[child].imageAlreadyComputed = vx_false_e;
				pyrInfo[child].pImage = data->children[child]->buffer;
			}
		}
	}
	else if (data->ref.type == VX_TYPE_IMAGE) {
		if (data->children) {
			for (vx_uint32 child = 0; child < data->numChildren; child++) {
				if (data->children[child]) {
					if (agoAllocData(data->children[child])) {
						// TBD error handling
						return -1;
					}
				}
			}
		}
		else if (data->u.img.isROI) {
            // make sure that the master image has been allocated
			if (!data->u.img.roiMasterImage->buffer) {
				if (agoAllocData(data->u.img.roiMasterImage) < 0) {
					return -1;
				}
			}
			// get the region from master image
			data->buffer = data->u.img.roiMasterImage->buffer +
				data->u.img.rect_roi.start_y * data->u.img.stride_in_bytes +
				((data->u.img.rect_roi.start_x * data->u.img.pixel_size_in_bits) >> 3);
		}
		else {
			if (data->u.img.isUniform) {
				// allocate buffer
				data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
				if (!data->buffer_allocated)
					return -1;
				// initialize image with uniform values
				if (data->u.img.format == VX_DF_IMAGE_RGBX) {
					vx_uint32 value = (data->u.img.uniform[0] & 0xff) | ((data->u.img.uniform[1] & 0xff) << 8) | ((data->u.img.uniform[2] & 0xff) << 16) | ((data->u.img.uniform[3] & 0xff) << 24);
					HafCpu_MemSet_U32(data->size >> 2, (vx_uint32 *)data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_RGB) {
					vx_uint32 value = (data->u.img.uniform[0] & 0xff) | ((data->u.img.uniform[1] & 0xff) << 8) | ((data->u.img.uniform[2] & 0xff) << 16);
					vx_uint8 * row = data->buffer;
					for (vx_uint32 y = 0; y < data->u.img.height; y++, row += data->u.img.stride_in_bytes) {
						HafCpu_MemSet_U24(data->u.img.width, row, value);
					}
				}
				else if (data->u.img.format == VX_DF_IMAGE_UYVY) {
					vx_uint32 value = (data->u.img.uniform[1] & 0xff) | ((data->u.img.uniform[0] & 0xff) << 8) | ((data->u.img.uniform[2] & 0xff) << 16) | ((data->u.img.uniform[0] & 0xff) << 24);
					HafCpu_MemSet_U32(data->size >> 2, (vx_uint32 *)data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_YUYV) {
					vx_uint32 value = (data->u.img.uniform[0] & 0xff) | ((data->u.img.uniform[1] & 0xff) << 8) | ((data->u.img.uniform[0] & 0xff) << 16) | ((data->u.img.uniform[2] & 0xff) << 24);
					HafCpu_MemSet_U32(data->size >> 2, (vx_uint32 *)data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_U8) {
					vx_uint8 value = (vx_uint8)data->u.img.uniform[0];
					HafCpu_MemSet_U8(data->size, data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_U16 || data->u.img.format == VX_DF_IMAGE_S16) {
					vx_uint16 value = (vx_uint16)data->u.img.uniform[0];
					HafCpu_MemSet_U16(data->size >> 1, (vx_uint16 *)data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_U32 || data->u.img.format == VX_DF_IMAGE_S32) {
					vx_uint32 value = (vx_uint32)data->u.img.uniform[0];
					HafCpu_MemSet_U32(data->size >> 2, (vx_uint32 *)data->buffer, value);
				}
				else if (data->u.img.format == VX_DF_IMAGE_U1_AMD) {
					vx_uint8 value = data->u.img.uniform[0] ? 255 : 0;
					HafCpu_MemSet_U8(data->size, data->buffer, value);
					// make sure that the data->u.img.uniform[0] is 0 or 1
					data->u.img.uniform[0] = data->u.img.uniform[0] ? 1 : 0;
				}
				else {
					// TBD error handling
					return -1;
				}
			}
			else {
				// allocate buffer and get aligned buffer with 16-byte alignment
				data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
				if (!data->buffer_allocated)
					return -1;
			}
		}
	}
	else if (data->ref.type == VX_TYPE_ARRAY) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_DISTRIBUTION) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		data->reserved = data->reserved_allocated = (vx_uint8 *)agoAllocMemory(256 * sizeof(vx_uint32));
		if (!data->buffer_allocated || !data->reserved_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_LUT) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_THRESHOLD) {
		// nothing to do
	}
	else if (data->ref.type == VX_TYPE_CONVOLUTION) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
		// allocate reserved buffer to store float version of coefficients
		data->reserved = data->reserved_allocated = (vx_uint8 *)agoAllocMemory(data->size << 1);
		if (!data->reserved_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_MATRIX) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_REMAP) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		data->reserved = data->reserved_allocated = (vx_uint8 *)agoAllocMemory(data->u.remap.dst_width * data->u.remap.dst_height * sizeof(ago_coord2d_float_t));
		if (!data->buffer_allocated || !data->reserved_allocated)
			return -1;
	}
	else if (data->ref.type == VX_TYPE_SCALAR) {
		// nothing to do
	}
	else if (data->ref.type == AGO_TYPE_MEANSTDDEV_DATA) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == AGO_TYPE_MINMAXLOC_DATA) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == AGO_TYPE_CANNY_STACK) {
		// allocate buffer and get aligned buffer with 16-byte alignment
		data->buffer = data->buffer_allocated = (vx_uint8 *)agoAllocMemory(data->size);
		if (!data->buffer_allocated)
			return -1;
	}
	else if (data->ref.type == AGO_TYPE_SCALE_MATRIX) {
		// nothing to do
	}
	else return -1;
	return 0;
}

void agoRetainData(AgoGraph * graph, AgoData * data, bool isForExternalUse)
{
	if (isForExternalUse) {
		data->ref.external_count++;
	}
	else {
		data->ref.internal_count++;
	}
	if (graph && data->isVirtual) {
		// if found in trash, move it to data list
		bool foundInTrash = false;
		if (data == graph->dataList.trash) {
			graph->dataList.trash = data->next;
			data->next = nullptr;
			foundInTrash = true;
		}
		else if (graph->dataList.trash) {
			for (AgoData * cur = graph->dataList.trash; cur->next; cur = cur->next) {
				if (cur->next == data) {
					cur->next = data->next;
					data->next = nullptr;
					foundInTrash = true;
					break;
				}
			}
		}
		if (foundInTrash) {
			// add the data into main part of the list
			data->next = graph->dataList.tail;
			graph->dataList.tail = data;
			if (!graph->dataList.head)
				graph->dataList.head = data;
		}
	}
}

int agoReleaseData(AgoData * data, bool isForExternalUse)
{
	if (data->isVirtual) {
		AgoGraph * graph = (AgoGraph *)data->ref.scope;
		CAgoLock lock(graph->cs);
		if (isForExternalUse) {
			if (data->ref.external_count > 0)
				data->ref.external_count--;
		}
		else {
			if (data->ref.internal_count > 0)
				data->ref.internal_count--;
		}
		if (data->ref.external_count == 0 && data->ref.internal_count == 0) {
			// clear child link in it's paren link
			if (data->parent) {
				for (vx_uint32 i = 0; i < data->parent->numChildren; i++) {
					if (data->parent->children[i] == data) {
						data->parent->children[i] = NULL;
					}
				}
			}
			// remove all children of data
			for (vx_uint32 i = 0; i < data->numChildren; i++) {
				if (data->children[i]) {
					// release the children
					data->children[i]->ref.external_count = 0;
					if (agoReleaseData(data->children[i], false)) {
						agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoReleaseData: agoReleaseData(context,%s) failed for children[%d]\n", data->children[i]->name.c_str(), i);
						return -1;
					}
					data->children[i] = NULL;
				}
			}
			// remove the data from graph
			if (agoRemoveData(&graph->dataList, data, nullptr)) {
				agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoReleaseData: agoRemoveData(context,%s) failed\n", data->name.c_str());
				return -1;
			}
		}
	}
	else {
		AgoContext * context = data->ref.context;
		CAgoLock lock(context->cs);
		if (isForExternalUse) {
			if (data->ref.external_count > 0)
				data->ref.external_count--;
		}
		else {
			if (data->ref.internal_count > 0)
				data->ref.internal_count--;
		}
		if (data->ref.external_count == 0 && data->ref.internal_count == 0) {
			// clear child link in it's paren link
			if (data->parent) {
				for (vx_uint32 i = 0; i < data->parent->numChildren; i++) {
					if (data->parent->children[i] == data) {
						data->parent->children[i] = NULL;
					}
				}
			}
			// remove all children of data
			for (vx_uint32 i = 0; i < data->numChildren; i++) {
				if (data->children[i]) {
					// release the children
					data->children[i]->ref.external_count = 0;
					data->children[i]->parent = NULL; // NOTE: this is needed to terminate recursion
					if (agoReleaseData(data->children[i], false)) {
						agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoReleaseData: agoReleaseData(context,%s) failed for children[%d]\n", data->children[i]->name.c_str(), i);
						return -1;
					}
					data->children[i] = NULL;
				}
			}
			// remove the data from context
			if (agoRemoveData(&context->dataList, data, nullptr)) {
				agoAddLogEntry(&data->ref, VX_FAILURE, "ERROR: agoReleaseData: agoRemoveData(context,%s) failed\n", data->name.c_str());
				return -1;
			}
		}
	}
	return 0;
}

int agoReleaseKernel(AgoKernel * kernel, bool isForExternalUse)
{
	vx_context context = kernel->ref.context;
	CAgoLock lock(context->cs);
	if (isForExternalUse) {
		if (kernel->ref.external_count > 0)
			kernel->ref.external_count--;
	}
	else {
		if (kernel->ref.internal_count > 0)
			kernel->ref.internal_count--;
	}
	if (kernel->ref.external_count == 0 && kernel->ref.internal_count == 0 && kernel->external_kernel && !kernel->finalized) {
		// only remove the kernels that are created externally
		if (agoRemoveKernel(&context->kernelList, kernel) != kernel) {
			agoAddLogEntry(&kernel->ref, VX_FAILURE, "ERROR: agoReleaseKernel: agoRemoveKernel(context,%s) failed\n", kernel->name);
			return -1;
		}
		delete kernel;
	}
	return 0;
}

AgoNode * agoCreateNode(AgoGraph * graph, AgoKernel * kernel)
{
	AgoNode * node = new AgoNode;
	agoResetReference(&node->ref, VX_TYPE_NODE, graph->ref.context, &graph->ref);
	node->attr_affinity = graph->attr_affinity;
	node->ref.internal_count = 1;
	node->akernel = kernel;
	node->attr_border_mode.mode = VX_BORDER_MODE_UNDEFINED;
	node->localDataSize = kernel->localDataSize;
	node->localDataPtr = NULL;
	node->paramCount = kernel->argCount;
	memcpy(node->parameters, kernel->parameters, sizeof(node->parameters));
	for (vx_uint32 i = 0; i < node->paramCount; i++) {
		agoResetReference(&node->parameters[i].ref, VX_TYPE_PARAMETER, graph->ref.context, &graph->ref);
		node->parameters[i].scope = &node->ref;
	}
	agoAddNode(&graph->nodeList, node);
	kernel->ref.internal_count++;
	return node;
}

AgoNode * agoCreateNode(AgoGraph * graph, vx_enum kernel_id)
{
	AgoNode * node = NULL;
	AgoKernel * kernel = agoFindKernelByEnum(graph->ref.context, kernel_id);
	if (kernel) {
		node = agoCreateNode(graph, kernel);
	}
	return node;
}

int agoReleaseNode(AgoNode * node)
{
	vx_graph graph = (vx_graph)node->ref.scope;
	CAgoLock lock(graph->cs);
	if (node->ref.external_count > 0) {
		node->ref.external_count--;
	}
	if (node->ref.external_count == 0 && node->ref.internal_count == 0) {
		// only remove the node if there are no internal references
		if (agoRemoveNode(&graph->nodeList, node, true)) {
			agoAddLogEntry(&node->akernel->ref, VX_FAILURE, "ERROR: agoReleaseNode: agoRemoveNode(graph,%s) failed\n", node->akernel->name);
			return -1;
		}
	}
	return 0;
}

void agoEvaluateIntegerExpression(char * expr)
{
	bool inValue = false;
	char opStack[32];
	int valStack[32], valStackTop = 0, opStackTop = 0;
	for (const char * s = expr; *s; s++) {
		char c = *s;
		if (c >= '0' && c <= '9') {
			if (!inValue) {
				inValue = true;
				valStack[valStackTop++] = 0;
			}
			valStack[valStackTop-1] = valStack[valStackTop-1] * 10 + c - '0';
		}
		else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')') {
			if (c == '(') {
				if (inValue) 
					return; // error
				opStack[opStackTop++] = c;
			}
			else {
				if (c == ')') {
					bool valid = false;
					for (; opStackTop-- > 0;) {
						if (opStack[opStackTop] == '(') {
							valid = true;
							break;
						}
						if (valStackTop < 2)
							return; // error
						valStackTop--;
						if (opStack[opStackTop] == '+') valStack[valStackTop - 1] += valStack[valStackTop];
						if (opStack[opStackTop] == '-') valStack[valStackTop - 1] -= valStack[valStackTop];
						if (opStack[opStackTop] == '*') valStack[valStackTop - 1] *= valStack[valStackTop];
						if (opStack[opStackTop] == '/') valStack[valStackTop - 1] /= valStack[valStackTop];
					}
					if (!valid)
						return; // error
				}
				else if (c == '+' || c == '-') {
					for (; opStackTop > 0;) {
						int op = opStack[opStackTop - 1];
						if (op == '+' || op == '-' || op == '*' || op == '/') {
							if (valStackTop < 2)
								return; // error
							opStackTop--;
							valStackTop--;
							if (opStack[opStackTop] == '+') valStack[valStackTop - 1] += valStack[valStackTop];
							if (opStack[opStackTop] == '-') valStack[valStackTop - 1] -= valStack[valStackTop];
							if (opStack[opStackTop] == '*') valStack[valStackTop - 1] *= valStack[valStackTop];
							if (opStack[opStackTop] == '/') valStack[valStackTop - 1] /= valStack[valStackTop];
						}
						else break;
					}
					opStack[opStackTop++] = c;
				}
				else if (c == '*' || c == '/') {
					for (; opStackTop > 0;) {
						int op = opStack[opStackTop - 1];
						if (op == '*' || op == '/') {
							if (valStackTop < 2)
								return; // error
							opStackTop--;
							valStackTop--;
							if (opStack[opStackTop] == '*') valStack[valStackTop - 1] *= valStack[valStackTop];
							if (opStack[opStackTop] == '/') valStack[valStackTop - 1] /= valStack[valStackTop];
						}
						else break;
					}
					opStack[opStackTop++] = c;
				}
			}
			inValue = false;
		}
		else
			return; // error
	}
	for (; opStackTop > 0;) {
		int op = opStack[opStackTop - 1];
		if (op == '+' || op == '-' || op == '*' || op == '/') {
			if (valStackTop < 2)
				return; // error
			opStackTop--;
			valStackTop--;
			if (opStack[opStackTop] == '+') valStack[valStackTop - 1] += valStack[valStackTop];
			if (opStack[opStackTop] == '-') valStack[valStackTop - 1] -= valStack[valStackTop];
			if (opStack[opStackTop] == '*') valStack[valStackTop - 1] *= valStack[valStackTop];
			if (opStack[opStackTop] == '/') valStack[valStackTop - 1] /= valStack[valStackTop];
		}
		else
			return; // error
	}
	if (valStackTop == 1) {
		sprintf(expr, "%d", valStack[0]);
	}
}

void agoImportNodeConfig(AgoNode * childnode, AgoNode * anode)
{
	childnode->attr_border_mode = anode->attr_border_mode;
	childnode->attr_affinity = anode->attr_affinity;
	if (anode->callback) {
		// TBD: need a mechanism to propagate callback changes later in the flow and
		// and ability to have multiple callbacks for the same node as multiple original nodes
		// can get mapped to one node after optimization and one can get mapped to several nodes 
		// after optimization
		childnode->callback = anode->callback;
	}
}

void agoPerfCaptureReset(vx_perf_t * perf)
{
	memset(perf, 0, sizeof(*perf));
}

void agoPerfCaptureStart(vx_perf_t * perf)
{
	perf->beg = agoGetClockCounter();
}

void agoPerfCaptureStop(vx_perf_t * perf)
{
	perf->end = agoGetClockCounter();
	perf->tmp = perf->end - perf->beg;
	perf->min = (perf->num == 0) ? perf->tmp : ((perf->tmp < perf->min) ? perf->tmp : perf->min);
	perf->max = (perf->num == 0) ? perf->tmp : ((perf->tmp > perf->max) ? perf->tmp : perf->max);
	perf->sum += perf->tmp;
	perf->num++;
	perf->avg = perf->sum / perf->num;
}

void agoPerfCopyNormalize(AgoContext * context, vx_perf_t * perfDst, vx_perf_t * perfSrc)
{
	agoPerfCaptureReset(perfDst);
	perfDst->num = perfSrc->num;
	perfDst->beg = perfSrc->beg;
	perfDst->end = perfSrc->end;
	perfDst->tmp = perfSrc->tmp;
	perfDst->sum = perfSrc->sum;
	perfDst->avg = perfSrc->avg;
	perfDst->min = perfSrc->min;
	perfDst->max = perfSrc->max;
}

void agoAddLogEntry(vx_reference ref, vx_status status, const char *message, ...)
{
	va_list ap;
	if (agoIsValidReference(ref) && ref->enable_logging && ref->context->callback_log) {
		vx_char string[VX_MAX_LOG_MESSAGE_LEN];
		va_start(ap, message);
		vsnprintf(string, VX_MAX_LOG_MESSAGE_LEN, message, ap);
		string[VX_MAX_LOG_MESSAGE_LEN - 1] = 0; // for MSVC which is not C99 compliant
		va_end(ap);
		if (!ref->context->callback_reentrant) {
			CAgoLock lock(ref->context->cs); // TBD: create a separate lock object for log_callback
			ref->context->callback_log(ref->context, ref, status, string);
		}
		else {
			ref->context->callback_log(ref->context, ref, status, string);
		}
	}
#if _DEBUG
	else {
		va_start(ap, message);
		vprintf(message, ap);
		va_end(ap);
	}
#endif
}

// constructor and destructors of basic data types
AgoReference::AgoReference()
: dispatchTbl{ nullptr }, magic{ AGO_MAGIC_VALID }, type{ VX_TYPE_REFERENCE }, context{ nullptr }, scope{ nullptr },
  external_count{ 0 }, internal_count{ 0 }, read_count{ 0 }, write_count{ 0 }, hint_serialize{ false }, enable_logging{ ENABLE_LOG_MESSAGES_DEFAULT },
  read_only{ false }, status{ VX_SUCCESS }
{
}
AgoReference::~AgoReference()
{
	magic = AGO_MAGIC_INVALID;
}
AgoData::AgoData()
	: next{ nullptr }, size{ 0 }, import_type{ VX_IMPORT_TYPE_NONE }, 
	  buffer{ nullptr }, buffer_allocated{ nullptr }, reserved{ nullptr }, reserved_allocated{ nullptr }, buffer_sync_flags{ 0 }, 
#if ENABLE_OPENCL
	  opencl_buffer{ nullptr }, opencl_buffer_allocated{ nullptr },
#endif
	  opencl_svm_buffer{ nullptr }, opencl_svm_buffer_allocated{ nullptr }, opencl_buffer_offset{ 0 },
	  isVirtual{ vx_false_e }, isDelayed{ vx_false_e }, isNotFullyConfigured{ vx_false_e }, isInitialized{ vx_false_e }, siblingIndex{ 0 },
	  numChildren{ 0 }, children{ nullptr }, parent{ nullptr }, inputUsageCount{ 0 }, outputUsageCount{ 0 }, inoutUsageCount{ 0 },
	  hierarchical_level{ 0 }, hierarchical_life_start{ 0 }, hierarchical_life_end{ 0 }
{
	memset(&u, 0, sizeof(u));
	memset(&delta, 0, sizeof(delta));
}
AgoData::~AgoData()
{
#if ENABLE_OPENCL
	agoGpuOclReleaseData(this);
#endif
	if (buffer_allocated) {
		agoReleaseMemory(buffer_allocated);
		buffer_allocated = nullptr;
	}
	if (reserved_allocated) {
		agoReleaseMemory(reserved_allocated);
		reserved_allocated = nullptr;
	}
}
AgoParameter::AgoParameter()
	: scope{ nullptr }, index{ 0 }, direction{ VX_INPUT }, type{ VX_TYPE_REFERENCE }, state{ VX_PARAMETER_STATE_REQUIRED }
{
}
AgoParameter::~AgoParameter()
{
}
AgoKernel::AgoKernel()
	: next{ nullptr }, id{ VX_KERNEL_INVALID }, flags{ 0 }, func{ nullptr }, argCount{ 0 }, kernOpType{ 0 }, kernOpInfo{ 0 },
	  localDataSize{ 0 }, localDataPtr{ nullptr }, external_kernel{ false }, finalized{ false },
	  kernel_f{ nullptr }, input_validate_f{ nullptr }, output_validate_f{ nullptr }, initialize_f{ nullptr }, deinitialize_f{ nullptr }, 
	  query_target_support_f{ nullptr }, opencl_codegen_callback_f{ nullptr }, regen_callback_f{ nullptr },
	  importing_module_index_plus1{ 0 }
{
	memset(&name, 0, sizeof(name));
	memset(&argConfig, 0, sizeof(argConfig));
	memset(&argType, 0, sizeof(argType));
}
AgoKernel::~AgoKernel()
{
}
AgoSuperNode::AgoSuperNode()
	: next{ nullptr }, group{ 0 }, width{ 0 }, height{ 0 }, launched{ false }, isGpuOclSuperNode{ false },
#if ENABLE_OPENCL
	  opencl_cmdq{ nullptr }, opencl_program{ nullptr }, opencl_kernel{ nullptr }, opencl_event{ nullptr },
#endif
	  status{ VX_SUCCESS }
{
#if ENABLE_OPENCL
	memset(&opencl_global_work, 0, sizeof(opencl_global_work));
#endif
	memset(&perf, 0, sizeof(perf));
}
AgoSuperNode::~AgoSuperNode()
{
}
AgoNode::AgoNode()
	: next{ nullptr }, akernel{ nullptr }, flags{ 0 }, localDataSize{ 0 }, localDataPtr{ nullptr }, localDataPtr_allocated{ nullptr },
	  paramCount{ 0 }, callback{ nullptr }, supernode{ nullptr }, initialized{ false }, target_support_flags{ 0 }, hierarchical_level{ 0 }, status{ VX_SUCCESS }
#if ENABLE_OPENCL
	, opencl_type{ 0 }, opencl_param_mem2reg_mask{ 0 }, opencl_param_discard_mask{ 0 }, 
	  opencl_param_atomic_mask{ 0 }, opencl_local_buffer_usage_mask{ 0 }, opencl_local_buffer_size_in_bytes{ 0 }, opencl_work_dim{ 0 },
	  opencl_compute_work_multiplier{ 0 }, opencl_compute_work_param_index{ 0 }, opencl_output_array_param_index_plus1{ 0 },
	  opencl_program{ nullptr }, opencl_kernel{ nullptr }, opencl_event{ nullptr }
#endif
{
	memset(&attr_border_mode, 0, sizeof(attr_border_mode));
	memset(&attr_affinity, 0, sizeof(attr_affinity));
	memset(&paramList, 0, sizeof(paramList));
	memset(&paramListForAgeDelay, 0, sizeof(paramListForAgeDelay));
	memset(&funcExchange, 0, sizeof(funcExchange));
	memset(&rect_valid, 0, sizeof(rect_valid));
	memset(&perf, 0, sizeof(perf));
#if ENABLE_OPENCL
	memset(&opencl_name, 0, sizeof(opencl_name));
	memset(&opencl_scalar_array_output_sync, 0, sizeof(opencl_scalar_array_output_sync));
	memset(&opencl_global_work, 0, sizeof(opencl_global_work));
	memset(&opencl_local_work, 0, sizeof(opencl_local_work));
#endif
}
AgoNode::~AgoNode()
{
	agoShutdownNode(this);
#if ENABLE_OPENCL
	if (opencl_event) {
		clReleaseEvent(opencl_event);
	}
	if (opencl_kernel) {
		clReleaseKernel(opencl_kernel);
	}
	if (opencl_program) {
		clReleaseProgram(opencl_program);
	}
#endif
}
AgoGraph::AgoGraph()
	: next{ nullptr }, hThread{ nullptr }, hSemToThread{ nullptr }, hSemFromThread{ nullptr },
	  threadScheduleCount{ 0 }, threadExecuteCount{ 0 }, threadWaitCount{ 0 }, threadThreadTerminationState{ 0 },
	  isReadyToExecute{ vx_false_e }, detectedInvalidNode{ false }, status{ VX_SUCCESS },
	  virtualDataGenerationCount{ 0 }, optimizer_flags{ AGO_GRAPH_OPTIMIZER_FLAGS_DEFAULT }, verified{ false }
#if ENABLE_OPENCL
	, supernodeList{ nullptr }, opencl_cmdq{ nullptr }, opencl_device{ nullptr }
#endif
{
	memset(&dataList, 0, sizeof(dataList));
	memset(&nodeList, 0, sizeof(nodeList));
	memset(&perf, 0, sizeof(perf));
	memset(&opencl_perf, 0, sizeof(opencl_perf));
	memset(&opencl_perf_total, 0, sizeof(opencl_perf_total));
	memset(&attr_affinity, 0, sizeof(attr_affinity));
	// critical section
	InitializeCriticalSection(&cs);
}
AgoGraph::~AgoGraph()
{
	// move all virtual data to garbage data list
	while (dataList.trash) {
		agoRemoveData(&dataList, dataList.trash, &ref.context->graph_garbage_data);
	}
	while (dataList.head) {
		agoRemoveData(&dataList, dataList.head, &ref.context->graph_garbage_data);
	}

	agoResetNodeList(&nodeList);
#if ENABLE_OPENCL
	agoResetSuperNodeList(supernodeList);
	supernodeList = NULL;
	agoGpuOclReleaseGraph(this);
#endif

	// critical section
	DeleteCriticalSection(&cs);
}
AgoContext::AgoContext()
	: perfNormFactor{ 0 }, dataGenerationCount{ 0 }, nextUserStructId{ VX_TYPE_USER_STRUCT_START },
	  num_active_modules{ 0 }, num_active_references{ 0 }, callback_log{ nullptr }, callback_reentrant{ vx_false_e },
	  thread_config{ CONFIG_THREAD_DEFAULT }, importing_module_index_plus1{ 0 }, graph_garbage_data{ nullptr }, graph_garbage_node{ nullptr }, graph_garbage_list{ nullptr }
#if ENABLE_OPENCL
	, opencl_context_imported{ false }, opencl_context{ nullptr }, opencl_cmdq{ nullptr }, opencl_config_flags{ 0 }, opencl_svmcaps{ 0 }, opencl_num_devices{ 0 }
#endif
{
	memset(&kernelList, 0, sizeof(kernelList));
	memset(&dataList, 0, sizeof(dataList));
	memset(&graphList, 0, sizeof(graphList));
	memset(&immediate_border_mode, 0, sizeof(immediate_border_mode));
	memset(&extensions, 0, sizeof(extensions));
#if ENABLE_OPENCL
	memset(&opencl_extensions, 0, sizeof(opencl_extensions));
	memset(&opencl_device_list, 0, sizeof(opencl_device_list));
	memset(&opencl_build_options, 0, sizeof(opencl_build_options));
#endif
	memset(&attr_affinity, 0, sizeof(attr_affinity));
	// critical section
	InitializeCriticalSection(&cs);
	// initialize constants as enumerations with name "!<name>"
	for (vx_uint32 i = 0; s_table_constants[i].name; i++) {
		char word[64];
		sprintf(word, "scalar:ENUM,0x%08x", s_table_constants[i].value);
		AgoData * data = agoCreateDataFromDescription(this, NULL, word, false);
		if (!data) {
			agoAddLogEntry(nullptr, VX_FAILURE, "ERROR: AgoContext::AgoContext: agoCreateDataFromDescription(*,%s) failed\n", word);
			ref.status = VX_FAILURE;
		}
		else {
			char name[256];
			name[0] = '!';
			strcpy(name + 1, s_table_constants[i].name);
			data->name = name;
			agoAddData(&dataList, data);
		}
	}
}
AgoContext::~AgoContext()
{
	for (AgoGraph * agraph = graphList.head; agraph;) {
		AgoGraph * next = agraph->next;
		agraph->ref.external_count = 1;
		agraph->ref.internal_count = 0;
		agoReleaseGraph(agraph);
		agraph = next;
	}

	agoResetDataList(&dataList);
	for (AgoData * data = graph_garbage_data; data;) {
		AgoData * item = data;
		data = data->next;
		delete item;
	}

	for (AgoNode * node = graph_garbage_node; node;) {
		AgoNode * item = node;
		node = node->next;
		delete item;
	}

	for (AgoGraph * graph = graph_garbage_list; graph;) {
		AgoGraph * item = graph;
		graph = graph->next;
		delete item;
	}

	for (auto it = macros.begin(); it != macros.end(); ++it) {
		if (it->text_allocated)
			free(it->text_allocated);
	}
	agoResetKernelList(&kernelList);
#if ENABLE_OPENCL
	agoGpuOclReleaseContext(this);
#endif
	// critical section
	DeleteCriticalSection(&cs);
}
