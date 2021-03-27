/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "twine/renderer/renderer.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "twine/menu/interface.h"
#include "twine/menu/menu.h"
#include "twine/renderer/redraw.h"
#include "twine/renderer/shadeangletab.h"
#include "twine/resources/resources.h"
#include "twine/scene/actor.h"
#include "twine/scene/grid.h"
#include "twine/scene/movements.h"
#include "twine/shared.h"
#include "twine/twine.h"

namespace TwinE {

#define RENDERTYPE_DRAWLINE 0
#define RENDERTYPE_DRAWPOLYGON 1
#define RENDERTYPE_DRAWSPHERE 2

Renderer::Renderer(TwinEEngine *engine) : _engine(engine) {
}

Renderer::~Renderer() {
	free(_polyTab);
	free(_polyTab2);
}

void Renderer::init(int32 w, int32 h) {
	_polyTabSize = _engine->height() * 6;
	_polyTab = (int16 *)malloc(_polyTabSize * sizeof(int16));
	_polyTab2 = (int16 *)malloc(_polyTabSize * sizeof(int16));
	_holomap_polytab_1_1 = &_polyTab[_engine->height() * 0];
	_holomap_polytab_1_2 = &_polyTab[_engine->height() * 2];
	_holomap_polytab_1_3 = &_polyTab[_engine->height() * 3];
	_holomap_polytab_2_3 = &_polyTab[_engine->height() * 5];
	_holomap_polytab_2_2 = &_polyTab[_engine->height() * 4];
	_holomap_polytab_2_1 = &_polyTab[_engine->height() * 1];
	_holomap_polytab_1_2_ptr = _holomap_polytab_1_2;
	_holomap_polytab_1_3_ptr = _holomap_polytab_1_3;
}

void Renderer::projectXYPositionOnScreen(int32 x, int32 y, int32 z) {
	if (_isUsingOrthoProjection == 1) {
		projPos.x = ((x - z) * 24) / BRICK_SIZE + _orthoProjPos.x;
		projPos.y = y;
		return;
	}
	int32 cz = baseRotPos.z - z;
	if (-1 < cz) {
		const int32 xdelta = x - baseRotPos.x;
		int32 posZ = cz + _cameraDepthOffset;
		if (posZ < 0) {
			posZ = 0x7FFF;
		}
		projPos.x = (xdelta * _cameraScaleY) / posZ + _orthoProjPos.x;
		projPos.y = y - baseRotPos.y;
		return;
	}
	projPos.x = 0;
	projPos.y = 0;
	return;
}

int32 Renderer::projectPositionOnScreen(int32 cX, int32 cY, int32 cZ) {
	if (_isUsingOrthoProjection) {
		projPos.x = ((cX - cZ) * 24) / BRICK_SIZE + _orthoProjPos.x;
		projPos.y = (((cX + cZ) * 12) - cY * 30) / BRICK_SIZE + _orthoProjPos.y;
		projPos.z = cZ - cY - cX;
		return 1;
	}

	cX -= baseRotPos.x;
	cY -= baseRotPos.y;
	cZ -= baseRotPos.z;

	if (cZ < 0) {
		projPos.x = 0;
		projPos.y = 0;
		projPos.z = 0;
		return 0;
	}

	int32 posZ = cZ + _cameraDepthOffset;
	if (posZ < 0) {
		posZ = 0x7FFF;
	}

	projPos.x = (cX * _cameraScaleY) / posZ + _orthoProjPos.x;
	projPos.y = (-cY * _cameraScaleZ) / posZ + _orthoProjPos.y;
	projPos.z = posZ;
	return -1;
}

void Renderer::setCameraPosition(int32 x, int32 y, int32 depthOffset, int32 scaleY, int32 scaleZ) {
	_orthoProjPos.x = x;
	_orthoProjPos.y = y;

	_cameraDepthOffset = depthOffset;
	_cameraScaleY = scaleY;
	_cameraScaleZ = scaleZ;

	_isUsingOrthoProjection = false;
}

void Renderer::setBaseTranslation(int32 x, int32 y, int32 z) {
	_baseTransPos.x = x;
	_baseTransPos.y = y;
	_baseTransPos.z = z;
}

void Renderer::setOrthoProjection(int32 x, int32 y, int32 z) {
	_orthoProjPos.x = x;
	_orthoProjPos.y = y;
	_orthoProjPos.z = z;

	_isUsingOrthoProjection = true;
}

void Renderer::baseMatrixTranspose() {
	SWAP(_baseMatrix.row1[1], _baseMatrix.row2[0]);
	SWAP(_baseMatrix.row1[2], _baseMatrix.row3[0]);
	SWAP(_baseMatrix.row2[2], _baseMatrix.row3[1]);
}

void Renderer::setBaseRotation(int32 x, int32 y, int32 z, bool transpose) {
	const double Xradians = (double)((ANGLE_90 - x) % ANGLE_360) * 2 * M_PI / ANGLE_360;
	const double Yradians = (double)((ANGLE_90 - y) % ANGLE_360) * 2 * M_PI / ANGLE_360;
	const double Zradians = (double)((ANGLE_90 - z) % ANGLE_360) * 2 * M_PI / ANGLE_360;

	_baseMatrix.row1[0] = (int32)(sin(Zradians) * sin(Yradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row1[1] = (int32)(-cos(Zradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row1[2] = (int32)(sin(Zradians) * cos(Yradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row2[0] = (int32)(cos(Zradians) * sin(Xradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row2[1] = (int32)(sin(Zradians) * sin(Xradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row3[0] = (int32)(cos(Zradians) * cos(Xradians) * SCENE_SIZE_HALFF);
	_baseMatrix.row3[1] = (int32)(sin(Zradians) * cos(Xradians) * SCENE_SIZE_HALFF);

	int32 matrixElem = _baseMatrix.row2[0];

	_baseMatrix.row2[0] = (int32)(sin(Yradians) * matrixElem + SCENE_SIZE_HALFF * cos(Yradians) * cos(Xradians));
	_baseMatrix.row2[2] = (int32)(cos(Yradians) * matrixElem - SCENE_SIZE_HALFF * sin(Yradians) * cos(Xradians));

	matrixElem = _baseMatrix.row3[0];

	_baseMatrix.row3[0] = (int32)(sin(Yradians) * matrixElem - SCENE_SIZE_HALFF * sin(Xradians) * cos(Yradians));
	_baseMatrix.row3[2] = (int32)(cos(Yradians) * matrixElem + SCENE_SIZE_HALFF * sin(Xradians) * sin(Yradians));

	if (transpose) {
		baseMatrixTranspose();
	}
	getBaseRotationPosition(_baseTransPos.x, _baseTransPos.y, _baseTransPos.z);

	baseRotPos.x = destPos.x;
	baseRotPos.y = destPos.y;
	baseRotPos.z = destPos.z;
}

void Renderer::getBaseRotationPosition(int32 x, int32 y, int32 z) {
	destPos.x = (_baseMatrix.row1[0] * x + _baseMatrix.row1[1] * y + _baseMatrix.row1[2] * z) / SCENE_SIZE_HALF;
	destPos.y = (_baseMatrix.row2[0] * x + _baseMatrix.row2[1] * y + _baseMatrix.row2[2] * z) / SCENE_SIZE_HALF;
	destPos.z = (_baseMatrix.row3[0] * x + _baseMatrix.row3[1] * y + _baseMatrix.row3[2] * z) / SCENE_SIZE_HALF;
}

void Renderer::getCameraAnglePositions(int32 x, int32 y, int32 z) {
	destPos.x = (_baseMatrix.row1[0] * x + _baseMatrix.row2[0] * y + _baseMatrix.row3[0] * z) / SCENE_SIZE_HALF;
	destPos.y = (_baseMatrix.row1[1] * x + _baseMatrix.row2[1] * y + _baseMatrix.row3[1] * z) / SCENE_SIZE_HALF;
	destPos.z = (_baseMatrix.row1[2] * x + _baseMatrix.row2[2] * y + _baseMatrix.row3[2] * z) / SCENE_SIZE_HALF;
}

void Renderer::translateGroup(int32 x, int32 y, int32 z) {
	destPos.x = (_shadeMatrix.row1[0] * x + _shadeMatrix.row1[1] * y + _shadeMatrix.row1[2] * z) / SCENE_SIZE_HALF;
	destPos.y = (_shadeMatrix.row2[0] * x + _shadeMatrix.row2[1] * y + _shadeMatrix.row2[2] * z) / SCENE_SIZE_HALF;
	destPos.z = destPos.y;
}

void Renderer::setCameraAngle(int32 transPosX, int32 transPosY, int32 transPosZ, int32 rotPosX, int32 rotPosY, int32 rotPosZ, int32 param6) {
	_baseTransPos.x = transPosX;
	_baseTransPos.y = transPosY;
	_baseTransPos.z = transPosZ;

	setBaseRotation(rotPosX, rotPosY, rotPosZ);

	baseRotPos.z += param6;

	getCameraAnglePositions(baseRotPos.x, baseRotPos.y, baseRotPos.z);

	_baseTransPos = destPos;
}

IVec3 Renderer::getHolomapRotation(const int32 angleX, const int32 angleY, const int32 angleZ) const {
	int32 rotX = angleX * 2 + 1000;

	int32 rotY;
	if (angleY == ANGLE_0) {
		rotY = ANGLE_0;
	} else {
		rotY = -shadeAngleTable[ClampAngle(angleY)] * rotX / SCENE_SIZE_HALF;
		rotX = shadeAngleTable[ClampAngle(angleY + ANGLE_90)] * rotX / SCENE_SIZE_HALF;
	}

	int32 rotZ;
	if (angleZ == ANGLE_0) {
		rotZ = ANGLE_0;
	} else {
		rotZ = -shadeAngleTable[ClampAngle(angleZ)] * rotX / SCENE_SIZE_HALF;
		rotX = shadeAngleTable[ClampAngle(angleZ + ANGLE_90)] * rotX / SCENE_SIZE_HALF;
	}

	const int32 row1X = _baseMatrix.row1[0] * rotX;
	const int32 row1Y = _baseMatrix.row1[1] * rotY;
	const int32 row1Z = _baseMatrix.row1[2] * rotZ;
	const int32 row2X = _baseMatrix.row2[0] * rotX;
	const int32 row2Y = _baseMatrix.row2[1] * rotY;
	const int32 row2Z = _baseMatrix.row2[2] * rotZ;
	const int32 row3X = _baseMatrix.row3[0] * rotX;
	const int32 row3Y = _baseMatrix.row3[1] * rotY;
	const int32 row3Z = _baseMatrix.row3[2] * rotZ;
	IVec3 vec;
	vec.x = (row1X + row1Y + row1Z) / SCENE_SIZE_HALF;
	vec.y = (row2X + row2Y + row2Z) / SCENE_SIZE_HALF;
	vec.z = (row3X + row3Y + row3Z) / SCENE_SIZE_HALF;
	return vec;
}

void Renderer::applyRotation(IMatrix3x3 *targetMatrix, const IMatrix3x3 *currentMatrix, const IVec3 &angleVec) {
	IMatrix3x3 matrix1;
	IMatrix3x3 matrix2;

	if (angleVec.x) {
		int32 angle = angleVec.x;
		int32 angleVar2 = shadeAngleTable[ClampAngle(angle)];
		angle += ANGLE_90;
		int32 angleVar1 = shadeAngleTable[ClampAngle(angle)];

		matrix1.row1[0] = currentMatrix->row1[0];
		matrix1.row2[0] = currentMatrix->row2[0];
		matrix1.row3[0] = currentMatrix->row3[0];

		matrix1.row1[1] = (currentMatrix->row1[2] * angleVar2 + currentMatrix->row1[1] * angleVar1) / SCENE_SIZE_HALF;
		matrix1.row1[2] = (currentMatrix->row1[2] * angleVar1 - currentMatrix->row1[1] * angleVar2) / SCENE_SIZE_HALF;
		matrix1.row2[1] = (currentMatrix->row2[2] * angleVar2 + currentMatrix->row2[1] * angleVar1) / SCENE_SIZE_HALF;
		matrix1.row2[2] = (currentMatrix->row2[2] * angleVar1 - currentMatrix->row2[1] * angleVar2) / SCENE_SIZE_HALF;
		matrix1.row3[1] = (currentMatrix->row3[2] * angleVar2 + currentMatrix->row3[1] * angleVar1) / SCENE_SIZE_HALF;
		matrix1.row3[2] = (currentMatrix->row3[2] * angleVar1 - currentMatrix->row3[1] * angleVar2) / SCENE_SIZE_HALF;
	} else {
		matrix1 = *currentMatrix;
	}

	if (angleVec.z) {
		int32 angle = angleVec.z;
		int32 angleVar2 = shadeAngleTable[ClampAngle(angle)];
		angle += ANGLE_90;
		int32 angleVar1 = shadeAngleTable[ClampAngle(angle)];

		matrix2.row1[2] = matrix1.row1[2];
		matrix2.row2[2] = matrix1.row2[2];
		matrix2.row3[2] = matrix1.row3[2];

		matrix2.row1[0] = (matrix1.row1[1] * angleVar2 + matrix1.row1[0] * angleVar1) / SCENE_SIZE_HALF;
		matrix2.row1[1] = (matrix1.row1[1] * angleVar1 - matrix1.row1[0] * angleVar2) / SCENE_SIZE_HALF;
		matrix2.row2[0] = (matrix1.row2[1] * angleVar2 + matrix1.row2[0] * angleVar1) / SCENE_SIZE_HALF;
		matrix2.row2[1] = (matrix1.row2[1] * angleVar1 - matrix1.row2[0] * angleVar2) / SCENE_SIZE_HALF;
		matrix2.row3[0] = (matrix1.row3[1] * angleVar2 + matrix1.row3[0] * angleVar1) / SCENE_SIZE_HALF;
		matrix2.row3[1] = (matrix1.row3[1] * angleVar1 - matrix1.row3[0] * angleVar2) / SCENE_SIZE_HALF;
	} else {
		matrix2 = matrix1;
	}

	if (angleVec.y) {
		int32 angle = angleVec.y;
		int32 angleVar2 = shadeAngleTable[ClampAngle(angle)];
		angle += ANGLE_90;
		int32 angleVar1 = shadeAngleTable[ClampAngle(angle)];

		targetMatrix->row1[1] = matrix2.row1[1];
		targetMatrix->row2[1] = matrix2.row2[1];
		targetMatrix->row3[1] = matrix2.row3[1];

		targetMatrix->row1[0] = (matrix2.row1[0] * angleVar1 - matrix2.row1[2] * angleVar2) / SCENE_SIZE_HALF;
		targetMatrix->row1[2] = (matrix2.row1[0] * angleVar2 + matrix2.row1[2] * angleVar1) / SCENE_SIZE_HALF;
		targetMatrix->row2[0] = (matrix2.row2[0] * angleVar1 - matrix2.row2[2] * angleVar2) / SCENE_SIZE_HALF;
		targetMatrix->row2[2] = (matrix2.row2[0] * angleVar2 + matrix2.row2[2] * angleVar1) / SCENE_SIZE_HALF;

		targetMatrix->row3[0] = (matrix2.row3[0] * angleVar1 - matrix2.row3[2] * angleVar2) / SCENE_SIZE_HALF;
		targetMatrix->row3[2] = (matrix2.row3[0] * angleVar2 + matrix2.row3[2] * angleVar1) / SCENE_SIZE_HALF;
	} else {
		*targetMatrix = matrix2;
	}
}

void Renderer::applyPointsRotation(const pointTab *pointsPtr, int32 numPoints, pointTab *destPoints, const IMatrix3x3 *rotationMatrix) {
	int32 numOfPoints2 = numPoints;

	do {
		const int32 tmpX = pointsPtr->x;
		const int32 tmpY = pointsPtr->y;
		const int32 tmpZ = pointsPtr->z;

		destPoints->x = ((rotationMatrix->row1[0] * tmpX + rotationMatrix->row1[1] * tmpY + rotationMatrix->row1[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.x;
		destPoints->y = ((rotationMatrix->row2[0] * tmpX + rotationMatrix->row2[1] * tmpY + rotationMatrix->row2[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.y;
		destPoints->z = ((rotationMatrix->row3[0] * tmpX + rotationMatrix->row3[1] * tmpY + rotationMatrix->row3[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.z;

		destPoints++;
		pointsPtr++;
	} while (--numOfPoints2);
}

void Renderer::processRotatedElement(IMatrix3x3 *targetMatrix, const pointTab *pointsPtr, int32 rotZ, int32 rotY, int32 rotX, const BonesBaseData *boneData, ModelData *modelData) {
	int32 firstPoint = boneData->firstPoint / sizeof(pointTab);
	int32 numOfPoints2 = boneData->numOfPoints;

	IVec3 renderAngle;
	renderAngle.x = rotX;
	renderAngle.y = rotY;
	renderAngle.z = rotZ;

	const IMatrix3x3 *currentMatrix;
	// if its the first point
	if (boneData->baseElement == -1) {
		currentMatrix = &_baseMatrix;

		destPos.x = 0;
		destPos.y = 0;
		destPos.z = 0;
	} else {
		const int32 pointIdx = boneData->basePoint / sizeof(pointTab);
		const int32 matrixIndex = boneData->baseElement / sizeof(BonesBaseData);
		assert(matrixIndex >= 0 && matrixIndex < ARRAYSIZE(_matricesTable));
		currentMatrix = &_matricesTable[matrixIndex];

		destPos.x = modelData->computedPoints[pointIdx].x;
		destPos.y = modelData->computedPoints[pointIdx].y;
		destPos.z = modelData->computedPoints[pointIdx].z;
	}

	applyRotation(targetMatrix, currentMatrix, renderAngle);

	if (!numOfPoints2) {
		warning("RENDER WARNING: No points in this model!");
	}

	applyPointsRotation(&pointsPtr[firstPoint], numOfPoints2, &modelData->computedPoints[firstPoint], targetMatrix);
}

void Renderer::applyPointsTranslation(const pointTab *pointsPtr, int32 numPoints, pointTab *destPoints, const IMatrix3x3 *translationMatrix, const IVec3 &angleVec) {
	int32 numOfPoints2 = numPoints;

	do {
		const int32 tmpX = pointsPtr->x + angleVec.z;
		const int32 tmpY = pointsPtr->y + angleVec.y;
		const int32 tmpZ = pointsPtr->z + angleVec.x;

		destPoints->x = ((translationMatrix->row1[0] * tmpX + translationMatrix->row1[1] * tmpY + translationMatrix->row1[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.x;
		destPoints->y = ((translationMatrix->row2[0] * tmpX + translationMatrix->row2[1] * tmpY + translationMatrix->row2[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.y;
		destPoints->z = ((translationMatrix->row3[0] * tmpX + translationMatrix->row3[1] * tmpY + translationMatrix->row3[2] * tmpZ) / SCENE_SIZE_HALF) + destPos.z;

		destPoints++;
		pointsPtr++;
	} while (--numOfPoints2);
}

void Renderer::processTranslatedElement(IMatrix3x3 *targetMatrix, const pointTab *pointsPtr, int32 rotX, int32 rotY, int32 rotZ, const BonesBaseData *boneData, ModelData *modelData) {
	IVec3 renderAngle;
	renderAngle.x = rotX;
	renderAngle.y = rotY;
	renderAngle.z = rotZ;

	if (boneData->baseElement == -1) { // base point
		destPos.x = 0;
		destPos.y = 0;
		destPos.z = 0;

		*targetMatrix = _baseMatrix;
	} else { // dependent
		const int32 pointsIdx = boneData->basePoint / sizeof(pointTab);
		destPos.x = modelData->computedPoints[pointsIdx].x;
		destPos.y = modelData->computedPoints[pointsIdx].y;
		destPos.z = modelData->computedPoints[pointsIdx].z;

		const int32 matrixIndex = boneData->baseElement / sizeof(BonesBaseData);
		assert(matrixIndex >= 0 && matrixIndex < ARRAYSIZE(_matricesTable));
		*targetMatrix = _matricesTable[matrixIndex];
	}

	applyPointsTranslation(&pointsPtr[boneData->firstPoint / sizeof(pointTab)], boneData->numOfPoints, &modelData->computedPoints[boneData->firstPoint / sizeof(pointTab)], targetMatrix, renderAngle);
}

void Renderer::setLightVector(int32 angleX, int32 angleY, int32 angleZ) {
	// TODO: RECHECK THIS
	/*_cameraAngleX = angleX;
	_cameraAngleY = angleY;
	_cameraAngleZ = angleZ;*/

	IVec3 renderAngle;
	renderAngle.x = angleX;
	renderAngle.y = angleY;
	renderAngle.z = angleZ;

	applyRotation(&_shadeMatrix, &_baseMatrix, renderAngle);
	translateGroup(0, 0, 59);

	_lightPos = destPos;
}

FORCEINLINE int16 clamp(int16 x, int16 a, int16 b) {
	return x < a ? a : (x > b ? b : x);
}

void Renderer::computeBoundingBox(Vertex *vertices, int32 numVertices, int &vleft, int &vright, int &vtop, int &vbottom) const {
	vleft = vtop = SCENE_SIZE_MAX;
	vright = vbottom = SCENE_SIZE_MIN;
	const int16 maxWidth = _engine->width() - 1;
	const int16 maxHeight = _engine->height() - 1;

	for (int32 i = 0; i < numVertices; i++) {
		vertices[i].x = clamp(vertices[i].x, 0, maxWidth);
		vleft = MIN<int>(vleft, vertices[i].x);
		vright = MAX<int>(vright, vertices[i].x);

		vertices[i].y = clamp(vertices[i].y, 0, maxHeight);
		vtop = MIN<int>(vtop, vertices[i].y);
		vbottom = MAX<int>(vbottom, vertices[i].y);
	}
}

void Renderer::computePolygons(int16 polyRenderType, const Vertex *vertices, int32 numVertices) {
	uint8 vertexParam1 = vertices[numVertices - 1].colorIndex;
	int16 currentVertexX = vertices[numVertices - 1].x;
	int16 currentVertexY = vertices[numVertices - 1].y;
	const int16 *polyTabBegin = _polyTab;
	const int16 *polyTabEnd = &_polyTab[_polyTabSize - 1];
	const int16 *polyTab2Begin = _polyTab2;
	const int16 *polyTab2End = &_polyTab2[_polyTabSize - 1];
	const int screenHeight = _engine->height();

	for (int32 nVertex = 0; nVertex < numVertices; nVertex++) {
		const int16 oldVertexY = currentVertexY;
		const int16 oldVertexX = currentVertexX;
		const uint8 oldVertexParam = vertexParam1;

		vertexParam1 = vertices[nVertex].colorIndex;
		const uint8 vertexParam2 = vertexParam1;
		currentVertexX = vertices[nVertex].x;
		currentVertexY = vertices[nVertex].y;

		// drawLine(oldVertexX,oldVertexY,currentVertexX,currentVertexY,255);

		if (currentVertexY == oldVertexY) {
			continue;
		}

		const int8 up = currentVertexY < oldVertexY;
		int8 direction = up ? -1 : 1;

		const int16 vsize = ABS(currentVertexY - oldVertexY);
		const int16 hsize = ABS(currentVertexX - oldVertexX);

		int16 cvalue;
		int16 cdelta;
		int16 ypos;
		float xpos;
		if (direction * oldVertexX > direction * currentVertexX) { // if we are going up right
			xpos = currentVertexX;
			ypos = currentVertexY;
			cvalue = (vertexParam2 * 256) + ((oldVertexParam - vertexParam2) * 256) % vsize;
			cdelta = ((oldVertexParam - vertexParam2) * 256) / vsize;
			direction = -direction; // we will draw by going down the tab
		} else {
			xpos = oldVertexX;
			ypos = oldVertexY;
			cvalue = (oldVertexParam * 256) + ((vertexParam2 - oldVertexParam) * 256) % vsize;
			cdelta = ((vertexParam2 - oldVertexParam) * 256) / vsize;
		}
		const int32 polyTabIndex = ypos + (up ? screenHeight : 0);
		int16 *outPtr = &_polyTab[polyTabIndex]; // outPtr is the output ptr in the renderTab

		float slope = (float)hsize / (float)vsize;
		slope = up ? -slope : slope;

		for (int16 i = 0; i < vsize + 2; i++) {
			if (outPtr >= polyTabBegin && outPtr <= polyTabEnd) {
				*outPtr = xpos;
			}
			outPtr += direction;
			xpos += slope;
		}

		if (polyRenderType >= POLYGONTYPE_GOURAUD) { // we must compute the color progression
			int16 *outPtr2 = &_polyTab2[polyTabIndex];

			for (int16 i = 0; i < vsize + 2; i++) {
				if (outPtr2 >= polyTab2Begin && outPtr2 <= polyTab2End) {
					*outPtr2 = cvalue;
				}
				outPtr2 += direction;
				cvalue += cdelta;
			}
		}
	}
}

void Renderer::renderPolygonsCopper(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();

	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		int16 start = ptr1[0];
		int16 stop = ptr1[screenHeight];

		ptr1++;
		int32 hsize = stop - start;

		if (hsize >= 0) {
			uint16 mask = 0x43DB;

			hsize++;
			const int32 startCopy = start;

			for (int32 j = startCopy; j < hsize + startCopy; j++) {
				start += mask;
				start = (start & 0xFF00) | ((start & 0xFF) & 3U);
				start = (start & 0xFF00) | ((start & 0xFF) + color);
				if (j >= 0 && j < screenWidth) {
					out[j] = start & 0xFF;
				}
				mask = (mask * 4) | (mask / SCENE_SIZE_HALF);
				mask++;
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsBopper(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();
	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		int16 start = ptr1[0];
		int16 stop = ptr1[screenHeight];
		ptr1++;
		const int32 hsize = stop - start;

		if (start & 1) {
			if (hsize >= 0) {
				for (int32 j = start; j <= hsize + start; j++) {
					if (j >= 0 && j < screenWidth) {
						out[j] = color;
					}
				}
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsFlat(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();
	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		const int16 start = ptr1[0];
		const int16 stop = ptr1[screenHeight];
		ptr1++;
		const int32 hsize = stop - start;

		if (hsize >= 0) {
			for (int32 j = start; j <= hsize + start; j++) {
				if (j >= 0 && j < screenWidth) {
					out[j] = color;
				}
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsTele(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	int bx = (uint16)color << 16;
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();

	int32 renderLoop = vsize;
	do {
		int16 start;
		int16 stop;
		int32 hsize;
		while (1) {
			start = ptr1[0];
			stop = ptr1[screenHeight];
			ptr1++;
			hsize = stop - start;

			if (hsize) {
				break;
			}

			uint8 *out2 = start + out;
			*out2 = ((uint16)(bx / 24)) & 0x0F;

			color = *(out2 + 1);

			out += screenWidth;

			--renderLoop;
			if (!renderLoop) {
				return;
			}
		}

		if (stop >= start) {
			hsize++;
			bx = (uint16)(color / 16);
			uint8 *out2 = start + out;

			int ax = (bx & 0xF0) * 256;
			bx = bx * 256;
			ax += (bx & 0x0F);
			ax -= bx;
			ax++;
			ax = ax >> 16;

			ax = ax / hsize;
			uint16 temp = (ax & 0xF0);
			temp = temp / 256;
			temp += (ax & 0x0F);
			ax = temp;

			uint16 dx = ax;

			ax = (ax & 0x0F) + (bx & 0xF0);
			hsize++;

			if (hsize & 1) {
				ax = 0; // not sure about this
			}

			for (int32 j = hsize >> 1; j > 0; --j) {
				*(out2++) = ax & 0x0F;
				ax += dx;

				*(out2++) = ax & 0x0F;
				ax += dx;
			}
		}

		out += screenWidth;
		--renderLoop;

	} while (renderLoop);
}

// FIXME: buggy
void Renderer::renderPolygonsTras(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();

	do {
		unsigned short int bx;

		int16 start = ptr1[0];
		int16 stop = ptr1[screenHeight];

		ptr1++;
		int32 hsize = stop - start;

		if (hsize >= 0) {
			hsize++;
			uint8 *out2 = start + out;

			if (hsize / 2 < 0) {
				bx = color;
				bx = bx * 256;
				bx += color;
				for (int32 j = 0; j < hsize; j++) {
					*(out2) = (*(out2)&0x0F0F) | bx;
					// TODO: check for potential out2++ here
				}
			} else {
				*out2 = (*(out2)&0x0F) | color;
				out2++;
			}
		}
		out += screenWidth;
	} while (--vsize);
}

// FIXME: buggy
void Renderer::renderPolygonsTrame(uint8 *out, int vtop, int32 vsize, uint8 color) const {
	const int16 *ptr1 = &_polyTab[vtop];
	unsigned char bh = 0;
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();

	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		int16 start = ptr1[0];
		int16 stop = ptr1[screenHeight];
		ptr1++;
		int32 hsize = stop - start;

		if (hsize >= 0) {
			hsize++;
			uint8 *out2 = start + out;

			hsize /= 2;
			if (hsize > 1) {
				uint16 ax;
				bh ^= 1;
				ax = (uint16)(*out2);
				ax &= 1;
				if (ax ^ bh) {
					out2++;
				}

				for (int32 j = 0; j < hsize; j++) {
					*out2 = color;
					out2 += 2;
				}
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsGouraud(uint8 *out, int vtop, int32 vsize) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int16 *ptr2 = &_polyTab2[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();
	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		uint16 startColor = ptr2[0];
		uint16 stopColor = ptr2[screenHeight];

		int16 colorSize = stopColor - startColor;

		int16 stop = ptr1[screenHeight]; // stop
		int16 start = ptr1[0];           // start

		ptr1++;
		uint8 *out2 = start + out;
		int32 hsize = stop - start;

		//varf2 = ptr2[screenHeight];
		//varf3 = ptr2[0];

		ptr2++;

		//varf4 = (float)((int32)varf2 - (int32)varf3);

		if (hsize == 0) {
			if (start >= 0 && start < screenWidth) {
				*out2 = ((startColor + stopColor) / 2) / 256; // moyenne des 2 couleurs
			}
		} else if (hsize > 0) {
			if (hsize == 1) {
				if (start >= -1 && start < screenWidth - 1) {
					*(out2 + 1) = stopColor / 256;
				}

				if (start >= 0 && start < screenWidth) {
					*(out2) = startColor / 256;
				}
			} else if (hsize == 2) {
				if (start >= -2 && start < screenWidth - 2) {
					*(out2 + 2) = stopColor / 256;
				}

				if (start >= -1 && start < screenWidth - 1) {
					*(out2 + 1) = ((startColor + stopColor) / 2) / 256;
				}

				if (start >= 0 && start < screenWidth) {
					*(out2) = startColor / 256;
				}
			} else {
				int32 currentXPos = start;
				colorSize /= hsize;
				hsize++;

				const uint8 startColorByte = startColor / 256;

				if (hsize % 2) {
					hsize /= 2;
					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*out2 = startColorByte;
					}
					out2++;
					currentXPos++;
					startColor += colorSize;
				} else {
					hsize /= 2;
				}

				do {
					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*out2 = startColorByte;
					}

					currentXPos++;
					startColor += colorSize;

					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*(out2 + 1) = startColorByte;
					}

					currentXPos++;
					out2 += 2;
					startColor += colorSize;
				} while (--hsize);
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsDither(uint8 *out, int vtop, int32 vsize) const {
	const int16 *ptr1 = &_polyTab[vtop];
	const int16 *ptr2 = &_polyTab2[vtop];
	const int screenWidth = _engine->width();
	const int screenHeight = _engine->height();

	int32 renderLoop = vsize;
	if (vtop < 0) {
		out += screenWidth * ABS(vtop);
		renderLoop -= ABS(vtop);
	}
	if (renderLoop > screenHeight) {
		renderLoop = screenHeight;
	}
	for (int32 currentLine = 0; currentLine < renderLoop; ++currentLine) {
		int16 stop = ptr1[screenHeight]; // stop
		int16 start = ptr1[0];           // start
		ptr1++;
		int32 hsize = stop - start;
		if (hsize < 0) {
			out += screenWidth;
			continue;
		}
		uint16 startColor = ptr2[0];
		uint16 stopColor = ptr2[screenHeight];
		int32 currentXPos = start;

		uint8 *out2 = start + out;
		ptr2++;

		if (hsize == 0) {
			if (currentXPos >= 0 && currentXPos < screenWidth) {
				*out2 = (uint8)(((startColor + stopColor) / 2) / 256);
			}
		} else {
			int16 colorSize = stopColor - startColor;
			if (hsize == 1) {
				uint16 currentColor = startColor;
				hsize++;
				hsize /= 2;

				currentColor &= 0xFF;
				currentColor += startColor;
				if (currentXPos >= 0 && currentXPos < screenWidth) {
					*out2 = currentColor / 256;
				}

				currentColor &= 0xFF;
				startColor += colorSize;
				currentColor = ((currentColor & (0xFF00)) | ((((currentColor & 0xFF) << (hsize & 0xFF))) & 0xFF));
				currentColor += startColor;

				currentXPos++;
				if (currentXPos >= 0 && currentXPos < screenWidth) {
					*(out2 + 1) = currentColor / 256;
				}
			} else if (hsize == 2) {
				uint16 currentColor = startColor;
				hsize++;
				hsize /= 2;

				currentColor &= 0xFF;
				colorSize /= 2;
				currentColor = ((currentColor & (0xFF00)) | ((((currentColor & 0xFF) << (hsize & 0xFF))) & 0xFF));
				currentColor += startColor;
				if (currentXPos >= 0 && currentXPos < screenWidth) {
					*out2 = currentColor / 256;
				}

				out2++;
				currentXPos++;
				startColor += colorSize;

				currentColor &= 0xFF;
				currentColor += startColor;

				if (currentXPos >= 0 && currentXPos < screenWidth) {
					*out2 = currentColor / 256;
				}

				currentColor &= 0xFF;
				startColor += colorSize;
				currentColor = ((currentColor & (0xFF00)) | ((((currentColor & 0xFF) << (hsize & 0xFF))) & 0xFF));
				currentColor += startColor;

				currentXPos++;
				if (currentXPos >= 0 && currentXPos < screenWidth) {
					*(out2 + 1) = currentColor / 256;
				}
			} else {
				uint16 currentColor = startColor;
				colorSize /= hsize;
				hsize++;

				if (hsize % 2) {
					hsize /= 2;
					currentColor &= 0xFF;
					currentColor = ((currentColor & (0xFF00)) | ((((currentColor & 0xFF) << (hsize & 0xFF))) & 0xFF));
					currentColor += startColor;
					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*out2 = currentColor / 256;
					}
					out2++;
					currentXPos++;
				} else {
					hsize /= 2;
				}

				do {
					currentColor &= 0xFF;
					currentColor += startColor;
					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*out2 = currentColor / 256;
					}
					currentXPos++;
					currentColor &= 0xFF;
					startColor += colorSize;
					currentColor = ((currentColor & (0xFF00)) | ((((currentColor & 0xFF) << (hsize & 0xFF))) & 0xFF));
					currentColor += startColor;
					if (currentXPos >= 0 && currentXPos < screenWidth) {
						*(out2 + 1) = currentColor / 256;
					}
					currentXPos++;
					out2 += 2;
					startColor += colorSize;
				} while (--hsize);
			}
		}
		out += screenWidth;
	}
}

void Renderer::renderPolygonsMarble(uint8 *out, int vtop, int32 vsize, uint8 color) const {
}

void Renderer::renderPolygons(const CmdRenderPolygon &polygon, Vertex *vertices) {
	int vleft = 0;
	int vright = 0;
	int vtop = 0;
	int vbottom = 0;

	if (polygon.numVertices > 0) {
		computeBoundingBox(vertices, polygon.numVertices, vleft, vright, vtop, vbottom);
	}
	computePolygons(polygon.renderType, vertices, polygon.numVertices);

	uint8 *out = (uint8 *)_engine->frontVideoBuffer.getBasePtr(0, vtop);
	const int32 vsize = vbottom - vtop + 1;

	switch (polygon.renderType) {
	case POLYGONTYPE_FLAT:
		renderPolygonsFlat(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_COPPER:
		renderPolygonsCopper(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_BOPPER:
		renderPolygonsBopper(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_TELE:
		renderPolygonsTele(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_TRAS:
		renderPolygonsTras(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_TRAME:
		renderPolygonsTrame(out, vtop, vsize, polygon.colorIndex);
		break;
	case POLYGONTYPE_GOURAUD:
		renderPolygonsGouraud(out, vtop, vsize);
		break;
	case POLYGONTYPE_DITHER:
		renderPolygonsDither(out, vtop, vsize);
		break;
	case POLYGONTYPE_MARBLE:
		renderPolygonsMarble(out, vtop, vsize, polygon.colorIndex);
		break;
	default:
		warning("RENDER WARNING: Unsupported render type %d", polygon.renderType);
		break;
	}
}

void Renderer::circleFill(int32 x, int32 y, int32 radius, uint8 color) {
	radius += 1;

	for (int32 currentLine = -radius; currentLine <= radius; currentLine++) {
		double width;

		if (ABS(currentLine) != radius) {
			width = ABS(sin(acos((float)currentLine / (float)radius)) * radius);
		} else {
			width = 0.0;
		}

		_engine->_interface->drawLine((int32)(x - width), currentLine + y, (int32)(x + width), currentLine + y, color);
	}
}

uint8 *Renderer::prepareSpheres(Common::MemoryReadStream &stream, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	int16 numSpheres = stream.readSint16LE();
	if (numSpheres <= 0) {
		return renderBufferPtr;
	}
	numOfPrimitives += numSpheres;
	do {
		CmdRenderSphere *sphere = (CmdRenderSphere *)renderBufferPtr;
		stream.skip(1);
		sphere->colorIndex = stream.readByte();
		stream.skip(2);
		sphere->radius = stream.readUint16LE();
		const int16 centerOffset = stream.readUint16LE();
		const int16 centerIndex = centerOffset / 6;
		sphere->x = modelData->flattenPoints[centerIndex].x;
		sphere->y = modelData->flattenPoints[centerIndex].y;

		(*renderCmds)->depth = modelData->flattenPoints[centerIndex].z;
		(*renderCmds)->renderType = RENDERTYPE_DRAWSPHERE;
		(*renderCmds)->dataPtr = renderBufferPtr;
		(*renderCmds)++;

		renderBufferPtr += sizeof(CmdRenderSphere);
	} while (--numSpheres);

	return renderBufferPtr;
}

uint8 *Renderer::prepareLines(Common::MemoryReadStream &stream, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	int16 numLines = stream.readSint16LE();
	if (numLines <= 0) {
		return renderBufferPtr;
	}
	numOfPrimitives += numLines;

	do {
		CmdRenderLine *lineCoordinatesPtr = (CmdRenderLine *)renderBufferPtr;
		lineCoordinatesPtr->colorIndex = stream.readByte();
		stream.skip(3);
		const int32 point1Index = stream.readSint16LE() / 6;
		const int32 point2Index = stream.readSint16LE() / 6;
		lineCoordinatesPtr->x1 = modelData->flattenPoints[point1Index].x;
		lineCoordinatesPtr->y1 = modelData->flattenPoints[point1Index].y;
		lineCoordinatesPtr->x2 = modelData->flattenPoints[point2Index].x;
		lineCoordinatesPtr->y2 = modelData->flattenPoints[point2Index].y;
		(*renderCmds)->depth = MAX(modelData->flattenPoints[point1Index].z, modelData->flattenPoints[point2Index].z);
		(*renderCmds)->renderType = RENDERTYPE_DRAWLINE;
		(*renderCmds)->dataPtr = renderBufferPtr;
		(*renderCmds)++;

		renderBufferPtr += sizeof(CmdRenderLine);
	} while (--numLines);

	return renderBufferPtr;
}

uint8 *Renderer::preparePolygons(Common::MemoryReadStream &stream, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	int16 numPolygons = stream.readSint16LE();
	if (numPolygons <= 0) {
		return renderBufferPtr;
	}
	int16 primitiveCounter = numPolygons; // the number of primitives = the number of polygons

	do { // loop that load all the polygons
		const uint8 renderType = stream.readByte();
		const uint8 numVertices = stream.readByte();
		assert(numVertices <= 16);
		const int16 colorIndex = stream.readSint16LE();

		int16 bestDepth = -32000;

		CmdRenderPolygon *destinationPolygon = (CmdRenderPolygon *)renderBufferPtr;
		destinationPolygon->numVertices = numVertices;

		renderBufferPtr += sizeof(CmdRenderPolygon);

		Vertex *const vertices = (Vertex *)renderBufferPtr;
		renderBufferPtr += destinationPolygon->numVertices * sizeof(Vertex);

		Vertex *vertex = vertices;
		int16 counter = destinationPolygon->numVertices;

		// TODO: RECHECK coordinates axis
		if (renderType >= 9) {
			destinationPolygon->renderType = renderType - 2;
			destinationPolygon->colorIndex = colorIndex;

			do {
				const int16 shadeEntry = stream.readSint16LE();
				const int16 shadeValue = colorIndex + modelData->shadeTable[shadeEntry];

				const int16 vertexOffset = stream.readSint16LE();
				const int16 vertexIndex = vertexOffset / 6;
				const pointTab *point = &modelData->flattenPoints[vertexIndex];

				vertex->colorIndex = shadeValue;
				vertex->x = point->x;
				vertex->y = point->y;
				bestDepth = MAX(bestDepth, point->z);
				++vertex;
			} while (--counter > 0);
		} else {
			if (renderType >= POLYGONTYPE_GOURAUD) {
				// only 1 shade value is used
				destinationPolygon->renderType = renderType - POLYGONTYPE_GOURAUD;
				const int16 shadeEntry = stream.readSint16LE();
				const int16 shadeValue = colorIndex + modelData->shadeTable[shadeEntry];
				destinationPolygon->colorIndex = shadeValue;
			} else {
				// no shade is used
				destinationPolygon->renderType = renderType;
				destinationPolygon->colorIndex = colorIndex;
			}

			do {
				const int16 vertexOffset = stream.readSint16LE();
				const int16 vertexIndex = vertexOffset / 6;
				const pointTab *point = &modelData->flattenPoints[vertexIndex];

				vertex->colorIndex = destinationPolygon->colorIndex;
				vertex->x = point->x;
				vertex->y = point->y;
				bestDepth = MAX(bestDepth, point->z);
				++vertex;
			} while (--counter > 0);
		}

		numOfPrimitives++;

		(*renderCmds)->depth = bestDepth;
		(*renderCmds)->renderType = RENDERTYPE_DRAWPOLYGON;
		(*renderCmds)->dataPtr = (uint8 *)destinationPolygon;
		(*renderCmds)++;
	} while (--primitiveCounter);

	return renderBufferPtr;
}

const Renderer::RenderCommand *Renderer::depthSortRenderCommands(int32 numOfPrimitives) {
	Common::sort(&_renderCmds[0], &_renderCmds[numOfPrimitives], [](const RenderCommand &lhs, const RenderCommand &rhs) { return lhs.depth > rhs.depth; });
	return _renderCmds;
}

bool Renderer::renderModelElements(int32 numOfPrimitives, const uint8 *polygonPtr, RenderCommand **renderCmds, ModelData *modelData) {
	// TODO: proper size
	Common::MemoryReadStream stream(polygonPtr, 100000);

	uint8 *renderBufferPtr = _renderCoordinatesBuffer;
	renderBufferPtr = preparePolygons(stream, numOfPrimitives, renderCmds, renderBufferPtr, modelData);
	renderBufferPtr = prepareLines(stream, numOfPrimitives, renderCmds, renderBufferPtr, modelData);
	renderBufferPtr = prepareSpheres(stream, numOfPrimitives, renderCmds, renderBufferPtr, modelData);

	if (numOfPrimitives == 0) {
		_engine->_redraw->renderRect.right = -1;
		_engine->_redraw->renderRect.bottom = -1;
		_engine->_redraw->renderRect.left = -1;
		_engine->_redraw->renderRect.top = -1;
		return false;
	}
	const RenderCommand *cmds = depthSortRenderCommands(numOfPrimitives);

	int16 primitiveCounter = numOfPrimitives;

	do {
		int16 type = cmds->renderType;
		uint8 *pointer = cmds->dataPtr;

		switch (type) {
		case RENDERTYPE_DRAWLINE: {
			const CmdRenderLine *lineCoords = (const CmdRenderLine *)pointer;
			const int32 x1 = lineCoords->x1;
			const int32 y1 = lineCoords->y1;
			const int32 x2 = lineCoords->x2;
			const int32 y2 = lineCoords->y2;
			_engine->_interface->drawLine(x1, y1, x2, y2, lineCoords->colorIndex);
			break;
		}
		case RENDERTYPE_DRAWPOLYGON: {
			const CmdRenderPolygon *header = (const CmdRenderPolygon *)pointer;
			Vertex *vertices = (Vertex *)(pointer + sizeof(CmdRenderPolygon));
			renderPolygons(*header, vertices);
			break;
		}
		case RENDERTYPE_DRAWSPHERE: {
			CmdRenderSphere *sphere = (CmdRenderSphere *)pointer;
			int32 radius = sphere->radius;

			//if (isUsingOrthoProjection) {
				radius = (radius * 34) / 512;
			//} else {
			//	radius = (radius * cameraScaleY) / (cameraDepthOffset + *(const int16 *)pointer); // TODO: this does not make sense.
			//}

			radius += 3;

			if (sphere->x + radius > _engine->_redraw->renderRect.right) {
				_engine->_redraw->renderRect.right = sphere->x + radius;
			}

			if (sphere->x - radius < _engine->_redraw->renderRect.left) {
				_engine->_redraw->renderRect.left = sphere->x - radius;
			}

			if (sphere->y + radius > _engine->_redraw->renderRect.bottom) {
				_engine->_redraw->renderRect.bottom = sphere->y + radius;
			}

			if (sphere->y - radius < _engine->_redraw->renderRect.top) {
				_engine->_redraw->renderRect.top = sphere->y - radius;
			}

			radius -= 3;

			circleFill(sphere->x, sphere->y, radius, sphere->colorIndex);
			break;
		}
		default:
			break;
		}

		cmds++;
	} while (--primitiveCounter);
	return true;
}

bool Renderer::renderAnimatedModel(ModelData *modelData, const uint8 *bodyPtr, RenderCommand *renderCmds, const IVec3 &angleVec, const IVec3 &renderPos) {
	const int32 numVertices = Model::getNumVertices(bodyPtr);
	const int32 numBones = Model::getNumBones(bodyPtr);

	const pointTab *pointsPtr = (const pointTab *)Model::getVerticesBaseData(bodyPtr);

	IMatrix3x3 *modelMatrix = &_matricesTable[0];

	const BonesBaseData *boneData = Model::getBonesBaseData(bodyPtr, 0);
	processRotatedElement(modelMatrix, pointsPtr, angleVec.x, angleVec.y, angleVec.z, boneData, modelData);

	int32 numOfPrimitives = 0;

	if (numBones - 1 != 0) {
		numOfPrimitives = numBones - 1;
		modelMatrix = &_matricesTable[1];

		int boneIdx = 1;
		do {
			boneData = Model::getBonesBaseData(bodyPtr, boneIdx);
			int16 boneType = boneData->flag;

			if (boneType == 0) {
				processRotatedElement(modelMatrix, pointsPtr, boneData->rotateX, boneData->rotateY, boneData->rotateZ, boneData, modelData);
			} else if (boneType == 1) {
				processTranslatedElement(modelMatrix, pointsPtr, boneData->rotateX, boneData->rotateY, boneData->rotateZ, boneData, modelData);
			}

			++modelMatrix;
			++boneIdx;
		} while (--numOfPrimitives);
	}

	numOfPrimitives = numVertices;

	const pointTab *pointPtr = &modelData->computedPoints[0];
	pointTab *pointPtrDest = &modelData->flattenPoints[0];

	if (_isUsingOrthoProjection) { // use standard projection
		do {
			const int32 coX = pointPtr->x + renderPos.x;
			const int32 coY = pointPtr->y + renderPos.y;
			const int32 coZ = -(pointPtr->z + renderPos.z);

			pointPtrDest->x = (coX + coZ) * 24 / BRICK_SIZE + _orthoProjPos.x;
			pointPtrDest->y = (((coX - coZ) * 12) - coY * 30) / BRICK_SIZE + _orthoProjPos.y;
			pointPtrDest->z = coZ - coX - coY;

			if (pointPtrDest->x < _engine->_redraw->renderRect.left) {
				_engine->_redraw->renderRect.left = pointPtrDest->x;
			}
			if (pointPtrDest->x > _engine->_redraw->renderRect.right) {
				_engine->_redraw->renderRect.right = pointPtrDest->x;
			}

			if (pointPtrDest->y < _engine->_redraw->renderRect.top) {
				_engine->_redraw->renderRect.top = pointPtrDest->y;
			}
			if (pointPtrDest->y > _engine->_redraw->renderRect.bottom) {
				_engine->_redraw->renderRect.bottom = pointPtrDest->y;
			}

			pointPtr++;
			pointPtrDest++;
		} while (--numOfPrimitives);
	} else {
		do {
			int32 coX = pointPtr->x + renderPos.x;
			int32 coY = pointPtr->y + renderPos.y;
			int32 coZ = -(pointPtr->z + renderPos.z);

			coZ += _cameraDepthOffset;

			if (coZ <= 0) {
				coZ = 0x7FFFFFFF;
			}

			// X projection
			{
				coX = _orthoProjPos.x + ((coX * _cameraScaleY) / coZ);

				if (coX > 0xFFFF) {
					coX = 0x7FFF;
				}

				pointPtrDest->x = coX;

				if (pointPtrDest->x < _engine->_redraw->renderRect.left) {
					_engine->_redraw->renderRect.left = pointPtrDest->x;
				}

				if (pointPtrDest->x > _engine->_redraw->renderRect.right) {
					_engine->_redraw->renderRect.right = pointPtrDest->x;
				}
			}

			// Y projection
			{
				coY = _orthoProjPos.y + ((-coY * _cameraScaleZ) / coZ);

				if (coY > 0xFFFF) {
					coY = 0x7FFF;
				}

				pointPtrDest->y = coY;

				if (pointPtrDest->y < _engine->_redraw->renderRect.top)
					_engine->_redraw->renderRect.top = pointPtrDest->y;
				if (pointPtrDest->y > _engine->_redraw->renderRect.bottom)
					_engine->_redraw->renderRect.bottom = pointPtrDest->y;
			}

			// Z projection
			{
				if (coZ > 0xFFFF) {
					coZ = 0x7FFF;
				}

				pointPtrDest->z = coZ;
			}

			pointPtr++;
			pointPtrDest++;

		} while (--numOfPrimitives);
	}

	int32 numOfShades = Model::getNumShades(bodyPtr);

	if (numOfShades) { // process normal data
		uint16 *currentShadeDestination = (uint16 *)modelData->shadeTable;
		IMatrix3x3 *lightMatrix = &_matricesTable[0];

		numOfPrimitives = numBones;

		int shadeIndex = 0;
		int boneIdx = 0;
		do { // for each element
			numOfShades = Model::getNumShadesBone(bodyPtr, boneIdx);

			if (numOfShades) {
				int32 numShades = numOfShades;

				_shadeMatrix = *lightMatrix * _lightPos;

				do { // for each normal
					const uint8 *shadePtr = Model::getShadesBaseData(bodyPtr, shadeIndex);
					const int16 *colPtr = (const int16 *)shadePtr;

					const int16 col1 = *((const int16 *)colPtr++);
					const int16 col2 = *((const int16 *)colPtr++);
					const int16 col3 = *((const int16 *)colPtr++);

					int32 color = 0;
					color += _shadeMatrix.row1[0] * col1 + _shadeMatrix.row1[1] * col2 + _shadeMatrix.row1[2] * col3;
					color += _shadeMatrix.row2[0] * col1 + _shadeMatrix.row2[1] * col2 + _shadeMatrix.row2[2] * col3;
					color += _shadeMatrix.row3[0] * col1 + _shadeMatrix.row3[1] * col2 + _shadeMatrix.row3[2] * col3;

					int32 shade = 0;

					if (color > 0) {
						color >>= 14;
						const uint8 *tmpShadePtr = (const uint8 *)shadePtr;
						color /= *((const uint16 *)(tmpShadePtr + 6));
						shade = (uint16)color;
					}

					*currentShadeDestination = shade;
					currentShadeDestination++;
					++shadeIndex;
				} while (--numShades);
			}

			++boneIdx;
			++lightMatrix;
		} while (--numOfPrimitives);
	}

	return renderModelElements(numOfPrimitives, Model::getPolygonData(bodyPtr), &renderCmds, modelData);
}

bool Renderer::renderIsoModel(int32 x, int32 y, int32 z, int32 angleX, int32 angleY, int32 angleZ, const uint8 *bodyPtr) {
	IVec3 renderAngle;
	renderAngle.x = angleX;
	renderAngle.y = angleY;
	renderAngle.z = angleZ;

	// model render size reset
	_engine->_redraw->renderRect.left = SCENE_SIZE_MAX;
	_engine->_redraw->renderRect.top = SCENE_SIZE_MAX;
	_engine->_redraw->renderRect.right = SCENE_SIZE_MIN;
	_engine->_redraw->renderRect.bottom = SCENE_SIZE_MIN;

	IVec3 renderPos;
	if (_isUsingOrthoProjection) {
		renderPos.x = x;
		renderPos.y = y;
		renderPos.z = z;
	} else {
		getBaseRotationPosition(x, y, z);

		renderPos = destPos - baseRotPos; // RECHECK y
	}

	if (!Model::isAnimated(bodyPtr)) {
		error("Unsupported unanimated model render!");
	}
	// restart at the beginning of the renderTable
	return renderAnimatedModel(&_modelData, bodyPtr, _renderCmds, renderAngle, renderPos);
}

void Renderer::renderBehaviourModel(const Common::Rect &rect, int32 y, int32 angle, const uint8 *bodyPtr) {
	renderBehaviourModel(rect.left, rect.top, rect.right, rect.bottom, y, angle, bodyPtr);
}

void Renderer::renderBehaviourModel(int32 boxLeft, int32 boxTop, int32 boxRight, int32 boxBottom, int32 y, int32 angle, const uint8 *bodyPtr) {
	const int32 ypos = (boxBottom + boxTop) / 2;
	const int32 xpos = (boxRight + boxLeft) / 2;

	setOrthoProjection(xpos, ypos, 0);
	_engine->_interface->setClip(Common::Rect(boxLeft, boxTop, boxRight, boxBottom));

	if (angle == -1) {
		ActorMoveStruct &move = _engine->_menu->moveMenu;
		const int16 newAngle = move.getRealAngle(_engine->lbaTime);
		if (move.numOfStep == 0) {
			_engine->_movements->setActorAngleSafe(newAngle, newAngle - ANGLE_90, ANGLE_17, &move);
		}
		renderIsoModel(0, y, 0, ANGLE_0, newAngle, ANGLE_0, bodyPtr);
	} else {
		renderIsoModel(0, y, 0, ANGLE_0, angle, ANGLE_0, bodyPtr);
	}
}

void Renderer::renderInventoryItem(int32 x, int32 y, const uint8 *bodyPtr, int32 angle, int32 param) {
	setCameraPosition(x, y, 128, 200, 200);
	setCameraAngle(0, 0, 0, 60, 0, 0, param);

	renderIsoModel(0, 0, 0, ANGLE_0, angle, ANGLE_0, bodyPtr);
}

void Renderer::computeHolomapPolygon(int32 top, int32 x1, int32 bottom, int32 x2, int16 *polygonTabPtr) {
	int32 minY = bottom;
	int32 minX = x1;
	if (top < bottom) {
		minY = top;
		top = bottom;
		minX = x2;
		x2 = x1;
	}
	const uint32 deltaY = top - minY;
	int16 *currentPolygonTabEntry = &polygonTabPtr[minY];
	if (minX < x2) {
		const uint32 deltaX = (x2 - minX) * 0x10000;
		const uint32 deltaRatio = deltaX / deltaY;
		uint32 iVar01 = (deltaRatio % deltaY >> 1) + 0x7fffU;
		for (uint32 y = 0; y <= deltaY; ++y) {
			if (currentPolygonTabEntry < _polyTab || currentPolygonTabEntry >= _polyTab + _polyTabSize) {
				currentPolygonTabEntry++;
				continue;
			}
			*currentPolygonTabEntry++ = (int16)x2;
			x2 -= (deltaRatio >> 0x10);
			if ((iVar01 & 0xffff0000U) != 0) {
				x2 += (iVar01 >> 0x10);
				iVar01 = iVar01 & 0xffffU;
			}
			iVar01 -= (deltaRatio & 0xffffU);
		}
	} else {
		const uint32 deltaX = (minX - x2) * 0x10000;
		const uint32 deltaRatio = deltaX / deltaY;
		uint32 iVar01 = (deltaX % deltaY >> 1) + 0x7fffU;
		for (uint32 y = 0; y <= deltaY; ++y) {
			if (currentPolygonTabEntry < _polyTab || currentPolygonTabEntry >= _polyTab + _polyTabSize) {
				currentPolygonTabEntry++;
				continue;
			}
			*currentPolygonTabEntry++ = (int16)x2;
			x2 += (deltaRatio >> 0x10);
			if ((iVar01 & 0xffff0000U) != 0) {
				x2 += (iVar01 >> 0x10);
				iVar01 = iVar01 & 0xffffU;
			}
			iVar01 += (deltaRatio & 0xffffU);
		}
	}
}

void Renderer::fillHolomapPolygons(const Vertex &vertex1, const Vertex &vertex2, const Vertex &angles1, const Vertex &angles2, int32 &top, int32 &bottom) {
	const int32 yBottom = vertex1.y;
	const int32 yTop = vertex2.y;
	if (yBottom == yTop) {
		return;
	}

	if (yBottom < yTop) {
		if (yBottom < top) {
			top = yBottom;
		}
		if (bottom < yTop) {
			bottom = yTop;
		}
		computeHolomapPolygon(yTop, vertex2.x, yBottom, vertex1.x, _holomap_polytab_1_1);
		computeHolomapPolygon(yTop, (uint32)(uint16)angles2.x, yBottom, (uint32)(uint16)angles1.x, _holomap_polytab_1_2);
	} else {
		if (bottom < yBottom) {
			bottom = yBottom;
		}
		if (yTop < top) {
			top = yTop;
		}
		computeHolomapPolygon(yTop, vertex2.x, yBottom, vertex1.x, _holomap_polytab_2_1);
		computeHolomapPolygon(yTop, (uint32)(uint16)angles2.x, yBottom, (uint32)(uint16)angles1.x, _holomap_polytab_2_2);
	}
	computeHolomapPolygon(yTop, (uint32)(uint16)angles2.y, yBottom, (uint32)(uint16)angles1.y, _holomap_polytab_2_3);
}

void Renderer::renderHolomapVertices(const Vertex vertexCoordinates[3], const Vertex vertexAngles[3]) {
	int32 top = 32000;
	int32 bottom = -32000;
	fillHolomapPolygons(vertexCoordinates[0], vertexCoordinates[1], vertexAngles[0], vertexAngles[1], top, bottom);
	fillHolomapPolygons(vertexCoordinates[1], vertexCoordinates[2], vertexAngles[1], vertexAngles[2], top, bottom);
	fillHolomapPolygons(vertexCoordinates[2], vertexCoordinates[0], vertexAngles[2], vertexAngles[0], top, bottom);
	renderHolomapPolygons(top, bottom);
}

void Renderer::renderHolomapPolygons(int32 top, int32 bottom) {
	const void *pixelBegin = _engine->frontVideoBuffer.getBasePtr(0, 0);
	const void *pixelEnd = _engine->frontVideoBuffer.getBasePtr(_engine->frontVideoBuffer.w - 1, _engine->frontVideoBuffer.h - 1);
	if (top < 0 || top >= _engine->frontVideoBuffer.h) {
		return;
	}
	uint8 *screenBufPtr = (uint8 *)_engine->frontVideoBuffer.getBasePtr(0, top);

	const int16 *lholomap_polytab_1_1 = _holomap_polytab_1_1 + top;
	const int16 *lholomap_polytab_2_1 = _holomap_polytab_2_1 + top;
	const uint16 *lholomap_polytab_1_2 = (const uint16 *)(_holomap_polytab_1_2 + top);
	const uint16 *lholomap_polytab_1_3 = (const uint16 *)(_holomap_polytab_1_3 + top);
	const uint16 *lholomap_polytab_2_2 = (const uint16 *)(_holomap_polytab_2_2 + top);
	const uint16 *lholomap_polytab_2_3 = (const uint16 *)(_holomap_polytab_2_3 + top);

	int32 yHeight = bottom - top;
	while (yHeight > -1) {
		const int16 left = *lholomap_polytab_1_1++;
		const int16 right = *lholomap_polytab_2_1++;
		const uint16 x_1_2 = *lholomap_polytab_1_2++;
		const uint16 x_1_3 = *lholomap_polytab_1_3++;
		const uint16 x_2_2 = *lholomap_polytab_2_2++;
		const uint16 x_2_3 = *lholomap_polytab_2_3++;
		int16 width = right - left;
		if (width > 0) {
			uint8 *pixelBufPtr = screenBufPtr + left;
			const int32 iWidth = (int32)width;
			uint32 uVar1 = (uint32)x_1_3;
			uint32 uVar3 = (uint32)x_1_2;
			for (int16 i = 0; i < width; ++i) {
				const uint32 holomapImageOffset = (uint32)((int32)uVar3 >> 8 & 0xffU) | (uVar1 & 0xff00);
				assert(holomapImageOffset < _engine->_resources->holomapImageSize);
				if (pixelBufPtr < pixelBegin || pixelBufPtr > pixelEnd) {
					++pixelBufPtr;
				} else {
					//debug("holomapImageOffset: %i", holomapImageOffset);
					*pixelBufPtr++ = _engine->_resources->holomapImagePtr[holomapImageOffset];
				}
				uVar1 += (int32)(((uint32)x_2_3 - (uint32)x_1_3) + 1) / iWidth;
				uVar3 += (int32)(((uint32)x_2_2 - (uint32)x_1_2) + 1) / iWidth;
			}
		}
		screenBufPtr += _engine->frontVideoBuffer.pitch;
		--yHeight;
	}
}

} // namespace TwinE
