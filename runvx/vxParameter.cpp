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


#define _CRT_SECURE_NO_WARNINGS

#include "vxParameter.h"
#include "vxParamHelper.h"
#include "vxArray.h"
#include "vxConvolution.h"
#include "vxDistribution.h"
#include "vxImage.h"
#include "vxLUT.h"
#include "vxMatrix.h"
#include "vxPyramid.h"
#include "vxRemap.h"
#include "vxScalar.h"
#include "vxThreshold.h"

#include <stdio.h>

#define DEBUG_INFO 0
#define DEBUG_FRAME 0
#define DEBUG_COMPARE 0
#define DEBUG_FILES 0

#define VX_MAX_FILE_NAME 128

////////////////////////////////////////
// parameter objects
CVxParameter::CVxParameter()
{
	// initialize local variables
	m_paramMap = nullptr;
	m_userStructMap = nullptr;
	m_vxObjType = VX_TYPE_INVALID;
	m_vxObjRef = nullptr;
	m_vxObjName[0] = '\0';
	m_fileNameForReadHasIndex = false;
	m_fileNameForWriteHasIndex = false;
	m_fileNameForCompareHasIndex = false;
	m_fpRead = nullptr;
	m_fpWrite = nullptr;
	m_fpCompare = nullptr;
	m_verbose = false;
	m_abortOnCompareMismatch = false;
	m_usingMultiFrameCapture = false;
	m_captureFrameStart = false;
}

CVxParameter::~CVxParameter()
{
	if (m_fpRead) fclose(m_fpRead);
	if (m_fpWrite) fclose(m_fpWrite);
	if (m_fpCompare) fclose(m_fpCompare);
}

const char * CVxParameter::GetVxObjectName()
{
	if (m_vxObjRef) {
		vxGetReferenceName(m_vxObjRef, m_vxObjName, sizeof(m_vxObjName));
	}
	return m_vxObjName;
}

void CVxParameter::DisableWaitForKeyPress()
{
}

void CVxParameter::ResetArrayListForView()
{
	m_arrayListForView.clear();
}

void CVxParameter::AddToArrayListForView(int colorIndex, int x, int y, float strength)
{
	if (m_displayName.length() > 0) {
		ArrayItemForView kpItem = { VX_TYPE_KEYPOINT, colorIndex, x, y, strength, 0, 0 };
		m_arrayListForView.push_back(kpItem);
	}
}

void CVxParameter::AddToArrayListForView(int colorIndex, int x, int y)
{
	if (m_displayName.length() > 0) {
		ArrayItemForView kpItem = { VX_TYPE_COORDINATES2D, colorIndex, x, y, 0.0f, 0, 0 };
		m_arrayListForView.push_back(kpItem);
	}
}

list<CVxParameter *> CVxParameter::m_paramList;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CVxParameter * CreateDataObject(vx_context context, vx_graph graph, std::map<std::string, CVxParameter *> * m_paramMap, map<string, vx_enum> * m_userStructMap, const char * desc, vx_uint32 captureFrameStart)
{
	// create the object based on the description
	if (!strncmp(desc, "image:", 6) || !strncmp(desc, "image-virtual:", 14) || !strncmp(desc, "image-uniform:", 14) || !strncmp(desc, "image-roi:", 10)) {
		CVxParamImage *this_image = new CVxParamImage();
		this_image->SetCaptureFrameStart(captureFrameStart);
		this_image->SetParamMap(m_paramMap);
		int status = this_image->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_image;
	}
	else if (!strncmp(desc, "array:", 6) || !strncmp(desc, "array-virtual:", 14)){
		CVxParamArray *this_array = new CVxParamArray();
		this_array->SetCaptureFrameStart(captureFrameStart);
		this_array->SetUserStructMap(m_userStructMap);
		int status = this_array->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_array;
	}
	else if (!strncmp(desc, "pyramid:", 8) || !strncmp(desc, "pyramid-virtual:", 16)) {
		CVxParamPyramid *this_pyramid = new CVxParamPyramid();
		this_pyramid->SetCaptureFrameStart(captureFrameStart);
		int status = this_pyramid->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_pyramid;
	}
	else if (!strncmp(desc, "distribution:", 13)){
		CVxParamDistribution *this_distribution = new CVxParamDistribution();
		this_distribution->SetCaptureFrameStart(captureFrameStart);
		int status = this_distribution->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_distribution;
	}
	else if (!strncmp(desc, "convolution:", 12)){
		CVxParamConvolution *this_convolution = new CVxParamConvolution();
		this_convolution->SetCaptureFrameStart(captureFrameStart);
		int status = this_convolution->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_convolution;
	}
	else if (!strncmp(desc, "lut:", 4)){
		CVxParamLUT *this_LUT = new CVxParamLUT();
		this_LUT->SetCaptureFrameStart(captureFrameStart);
		int status = this_LUT->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_LUT;
	}
	else if (!strncmp(desc, "matrix:", 7)){
		CVxParamMatrix *this_matrix = new CVxParamMatrix();
		this_matrix->SetCaptureFrameStart(captureFrameStart);
		int status = this_matrix->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_matrix;
	}
	else if (!strncmp(desc, "remap:", 6)){
		CVxParamRemap *this_remap = new CVxParamRemap();
		this_remap->SetCaptureFrameStart(captureFrameStart);
		int status = this_remap->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_remap;
	}
	else if (!strncmp(desc, "scalar:", 7) || !strncmp(desc, "!", 1)){
		if (!strncmp(desc, "!", 1)){
			char enum_name[2048];
			char description[2048];
			char desc2[2048];
			int i = 1;
			int j = 0;
			while (desc[i] != '\0'){
				enum_name[j] = desc[i];
				i++;
				j++;
			}
			enum_name[j] = '\0';
			strcpy(description, "scalar:enum,%s");
			sprintf(desc2, description, enum_name);
			//printf("DEBUG: %s\n", desc3);
			CVxParamScalar *this_scalar = new CVxParamScalar();
			this_scalar->SetCaptureFrameStart(captureFrameStart);
			int status = this_scalar->Initialize(context, graph, desc2);
			if (status)
				return NULL;
			return this_scalar;
		}
		else {
			CVxParamScalar *this_scalar = new CVxParamScalar();
			this_scalar->SetCaptureFrameStart(captureFrameStart);
			int status = this_scalar->Initialize(context, graph, desc);
			if (status)
				return NULL;
			return this_scalar;
		}
	}
	else if (!strncmp(desc, "threshold:", 10)){
		CVxParamThreshold *this_threshold = new CVxParamThreshold();
		this_threshold->SetCaptureFrameStart(captureFrameStart);
		int status = this_threshold->Initialize(context, graph, desc);
		if (status)
			return NULL;
		return this_threshold;
	}
	else return nullptr;
}

CVxParameter * CreateDataObject(vx_context context, vx_graph graph, vx_reference ref, const char * params, vx_uint32 captureFrameStart)
{
	// create the object based on the ref
	vx_enum type;
	vx_status status = vxQueryReference(ref, VX_REF_ATTRIBUTE_TYPE, &type, sizeof(type));
	if (status) {
		printf("ERROR: CreateDataObject: vxQueryReference(*,VX_REF_ATTRIBUTE_TYPE,...) failed(%d)\n", status);
		throw -1;
	}
	if (type == VX_TYPE_IMAGE) {
		CVxParamImage *this_image = new CVxParamImage();
		this_image->SetCaptureFrameStart(captureFrameStart);
		if (this_image->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_image;
	}
	else if (type == VX_TYPE_ARRAY) {
		CVxParamArray *this_array = new CVxParamArray();
		this_array->SetCaptureFrameStart(captureFrameStart);
		if (this_array->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_array;
	}
	else if (type == VX_TYPE_PYRAMID) {
		CVxParamPyramid *this_pyramid = new CVxParamPyramid();
		this_pyramid->SetCaptureFrameStart(captureFrameStart);
		if (this_pyramid->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_pyramid;
	}
	else if (type == VX_TYPE_DISTRIBUTION) {
		CVxParamDistribution *this_distribution = new CVxParamDistribution();
		this_distribution->SetCaptureFrameStart(captureFrameStart);
		if (this_distribution->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_distribution;
	}
	else if (type == VX_TYPE_CONVOLUTION) {
		CVxParamConvolution *this_convolution = new CVxParamConvolution();
		this_convolution->SetCaptureFrameStart(captureFrameStart);
		if (this_convolution->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_convolution;
	}
	else if (type == VX_TYPE_LUT) {
		CVxParamLUT *this_LUT = new CVxParamLUT();
		this_LUT->SetCaptureFrameStart(captureFrameStart);
		if (this_LUT->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_LUT;
	}
	else if (type == VX_TYPE_MATRIX) {
		CVxParamMatrix *this_matrix = new CVxParamMatrix();
		this_matrix->SetCaptureFrameStart(captureFrameStart);
		if (this_matrix->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_matrix;
	}
	else if (type == VX_TYPE_REMAP) {
		CVxParamRemap *this_remap = new CVxParamRemap();
		this_remap->SetCaptureFrameStart(captureFrameStart);
		if (this_remap->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_remap;
	}
	else if (type == VX_TYPE_SCALAR) {
		CVxParamScalar *this_scalar = new CVxParamScalar();
		this_scalar->SetCaptureFrameStart(captureFrameStart);
		if (this_scalar->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_scalar;
	}
	else if (type == VX_TYPE_THRESHOLD) {
		CVxParamThreshold *this_threshold = new CVxParamThreshold();
		this_threshold->SetCaptureFrameStart(captureFrameStart);
		if (this_threshold->InitializeIO(context, graph, ref, params))
			return NULL;
		return this_threshold;
	}
	else return nullptr;
}

/*! \brief Parse parameter strings.
* \details This creates a top-level object context for OpenVX.
* \param [in] s The input string.
* \param [in] syntax The syntax description for error messaging.
* \param [in] fmt The format string: d(32-bit integer) D(64-bit integer) f(float) F(double) c(color-format) s(string upto 64-chars) S(string upto 256-chars).
* \param [in] ... Pointers to list of parameters.
* \return pointer to input string after processing the all the parameters
*/
const char * ScanParameters(const char * s_, const char * syntax, const char * fmt_, ...)
{
	va_list argp;
	va_start(argp, fmt_);
	const char *s = s_;
	for (const char * fmt = fmt_; *fmt != '\0'; fmt++) {
		const char * t = s;
		if (*s != '\0') {
			if (*fmt == 'd' || *fmt == 'D') { // 32-bit/64-bit integer in decimal or hexadecimal
				int64_t value = 0;
				if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
					// parse hexadecimal: 0x...
					s += 2;
					for (; (*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'); s++) {
						if (*s >= 'a' && *s <= 'f') {
							value = value * 16 + *s - 'a' + 10;
						}
						else if (*s >= 'A' && *s <= 'F') {
							value = value * 16 + *s - 'A' + 10;
						}
						else {
							value = value * 16 + *s - '0';
						}
					}
				}
				else {
					// parse decimal string
					int sign = 1;
					if (*s == '-') {
						sign = -1;
						s++;
					}
					for (; *s >= '0' && *s <= '9'; s++)
						value = value * 10 + *s - '0';
					value *= sign;
				}
				if (*fmt == 'd') {
					// 32-bit integer
					*(va_arg(argp, int32_t *)) = (int32_t)value;
				}
				else {
					// 64-bit integer
					*(va_arg(argp, int64_t *)) = value;
				}
			}
			else if (*fmt == 'f' || *fmt == 'F') { // 32-bit/64-bit floating-point
				char buf[64] = { 0 };
				for (int i = 0; i < ((int)sizeof(buf) - 1) && ((*s >= '0' && *s <= '9') || *s == '.' || *s == '-' || *s == 'e'); i++)
					buf[i] = *s++;
				if (*fmt == 'f') {
					// 32-bit float
					*(va_arg(argp, float *)) = (float)atof(buf);
				}
				else {
					// 64-bit double
					*(va_arg(argp, double *)) = atof(buf);
				}
			}
			else if (*fmt == 'c') { // color format
				if (s[0] && s[1] && s[2] && s[3]) {
					*(va_arg(argp, vx_df_image *)) = (vx_df_image)VX_DF_IMAGE(s[0], s[1], s[2], s[3]);
					s += 4;
				}
			}
			else if (*fmt == 's' || *fmt == 'S') { // string of upto 64-bytes/256-bytes until ',', ':', or end-of-string
				int maxStringBufferLength = (*fmt == 'S') ? 256 : 64;
				char * p = va_arg(argp, char *);
				if (!_strnicmp(s, "https://", 8) || !_strnicmp(s, "http://", 7) || !_strnicmp(s, "file://", 7) || 
					(((s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z')) && s[1] == ':' && s[2] == '\\'))
				{
					// started with drive letter or url, so copy prefix string to avoid use of ':' as end marker
					int len = (s[1] == ':') ? 3 : ((s[4] == ':') ? 7 : 8);
					strncpy(p, s, len);
					p += len;  s += len;
					maxStringBufferLength -= len;
				}
				// copy till end of string or ',' or ':'
				for (; (*s != '\0') && (*s != ',') && (*s != ':') && (--maxStringBufferLength > 0);)
					*p++ = *s++;
				*p = 0;
			}
			else if (*fmt == *s) { // skip matched seperators in fmt
				s++;
			}
		}
		// check to make sure that at least one character from input has been used for parsing the current parameter
		if (s == t) {
			printf("ERROR: ScanParameters: invalid string syntax=[%s] fmt=[%s] s=[%s]\n", syntax, fmt_, s_);
			throw - 1;
		}
	}
	va_end(argp);
	return s;
}
