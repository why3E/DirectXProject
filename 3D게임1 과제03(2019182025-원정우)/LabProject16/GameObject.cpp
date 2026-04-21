#include "GameObject.h"
#include "Shader.h"


inline float RandF(float fMin, float fMax)
{
	return(fMin + ((float)rand() / (float)RAND_MAX) * (fMax - fMin));
}

XMVECTOR RandomUnitVectorOnSphere()
{
	XMVECTOR xmvOne = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR xmvZero = XMVectorZero();

	while (true)
	{
		XMVECTOR v = XMVectorSet(RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), 0.0f);
		if (!XMVector3Greater(XMVector3LengthSq(v), xmvOne)) return(XMVector3Normalize(v));
	}
}


CGameObject::CGameObject()
{
	XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());
}

CGameObject::~CGameObject()
{
	if (m_pMesh) m_pMesh->Release();
	if (m_pShader)
	{
		m_pShader->ReleaseShaderVariables();
		m_pShader->Release();
	}
}

void CGameObject::OnPrepareRender()
{

}

bool CGameObject::IsVisible(CCamera* pCamera)
{
	OnPrepareRender();
	bool bIsVisible = false;

	BoundingOrientedBox xmBoundingBox = m_pMesh->m_xmOOBB;
	xmBoundingBox.Transform(xmBoundingBox, XMLoadFloat4x4(&m_xmf4x4World));

	if (pCamera) bIsVisible = pCamera->IsInFrustum(xmBoundingBox);
	return(bIsVisible);
}

void CGameObject::SetShader(CShader* pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();

}
void CGameObject::SetMesh(CMesh* pMesh)
{
	if (m_pMesh) m_pMesh->Release();
	m_pMesh = pMesh;
	if (m_pMesh) m_pMesh->AddRef();
}

void CGameObject::ReleaseUploadBuffers()
{
	//정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();
}

void CGameObject::Animate(float fTimeElapsed)
{
	if (m_fMovingSpeed != 0.0f) Move(m_xmf3MovingDirection, m_fMovingSpeed * fTimeElapsed);
	
	UpdateBoundingBox();

	if (type == 'A') {
		TargetRotate();
	}
}

void CGameObject::UpdateBoundingBox()
{
	if (m_pMesh)
	{
		m_pMesh->m_xmOOBB.Transform(m_xmOOBB, XMLoadFloat4x4(&m_xmf4x4World));
		XMStoreFloat4(&m_xmOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_xmOOBB.Orientation)));
	}
}

void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice,
	ID3D12GraphicsCommandList* pd3dCommandList)
{
}
void CGameObject::ReleaseShaderVariables()
{
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	//객체의 월드 변환 행렬을 루트 상수(32-비트 값)를 통하여 셰이더 변수(상수 버퍼)로 복사한다.
	pd3dCommandList->SetGraphicsRoot32BitConstants(0, 16, &xmf4x4World, 0);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{

	//게임 객체가 카메라에 보이면 렌더링한다..
	if (m_pMesh) {
		if (IsVisible(pCamera))
		{
			UpdateShaderVariables(pd3dCommandList);
			if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera);
			if (m_pMesh) m_pMesh->Render(pd3dCommandList);
		}
	}
}


void CGameObject::Rotate(XMFLOAT3* pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis),
		XMConvertToRadians(fAngle));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::Move(XMFLOAT3& vDirection, float fSpeed)
{
	SetPosition(m_xmf4x4World._41 + vDirection.x * fSpeed, m_xmf4x4World._42 + vDirection.y * fSpeed, m_xmf4x4World._43 + vDirection.z * fSpeed);

}


CRotatingObject::CRotatingObject()
{

}
CRotatingObject::~CRotatingObject()
{
}
void CRotatingObject::Animate(float fTimeElapsed)
{

	CGameObject::Rotate(&m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed);
	CGameObject::Animate(fTimeElapsed);

}
void CRotatingObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CGameObject::Render(pd3dCommandList, pCamera);
}
void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4World._41 = x;
	m_xmf4x4World._42 = y;
	m_xmf4x4World._43 = z;

}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);

}
XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	XMFLOAT3 xmf3LookAt(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33);
	xmf3LookAt = Vector3::Normalize(xmf3LookAt);
	return(xmf3LookAt);
}

XMFLOAT3 CGameObject::GetUp()
{
	XMFLOAT3 xmf3Up(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23);
	xmf3Up = Vector3::Normalize(xmf3Up);
	return(xmf3Up);
}

XMFLOAT3 CGameObject::GetRight()
{
	XMFLOAT3 xmf3Right(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13);
	xmf3Right = Vector3::Normalize(xmf3Right);
	return(xmf3Right);
}
//게임 객체를 로컬 x-축 방향으로 이동한다.
void CGameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	CGameObject::SetPosition(xmf3Position);
}
//게임 객체를 로컬 y-축 방향으로 이동한다.
void CGameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	CGameObject::SetPosition(xmf3Position);
}
//게임 객체를 로컬 z-축 방향으로 이동한다.
void CGameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	CGameObject::SetPosition(xmf3Position);
}
//게임 객체를 주어진 각도로 회전한다.
void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch),
		XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

//모델 좌표계의 픽킹 광선을 생성한다.-----------------------------------------------------------------
void CGameObject::GenerateRayForPicking(XMFLOAT3& xmf3PickPosition, XMFLOAT4X4&
	xmf4x4View, XMFLOAT3* pxmf3PickRayOrigin, XMFLOAT3* pxmf3PickRayDirection)
{
	XMFLOAT4X4 xmf4x4WorldView = Matrix4x4::Multiply(m_xmf4x4World, xmf4x4View);
	XMFLOAT4X4 xmf4x4Inverse = Matrix4x4::Inverse(xmf4x4WorldView);
	XMFLOAT3 xmf3CameraOrigin(0.0f, 0.0f, 0.0f);
	//카메라 좌표계의 원점을 모델 좌표계로 변환한다.
	*pxmf3PickRayOrigin = Vector3::TransformCoord(xmf3CameraOrigin, xmf4x4Inverse);
	//카메라 좌표계의 점(마우스 좌표를 역변환하여 구한 점)을 모델 좌표계로 변환한다.
	*pxmf3PickRayDirection = Vector3::TransformCoord(xmf3PickPosition, xmf4x4Inverse);
	//광선의 방향 벡터를 구한다.
	*pxmf3PickRayDirection = Vector3::Normalize(Vector3::Subtract(*pxmf3PickRayDirection,
		*pxmf3PickRayOrigin));
}

int CGameObject::PickObjectByRayIntersection(XMFLOAT3& xmf3PickPosition, XMFLOAT4X4&
	xmf4x4View, float* pfHitDistance)
{
	int nIntersected = 0;
	if (m_pMesh)
	{
		XMFLOAT3 xmf3PickRayOrigin, xmf3PickRayDirection;
		//모델 좌표계의 광선을 생성한다.
		GenerateRayForPicking(xmf3PickPosition, xmf4x4View, &xmf3PickRayOrigin,
			&xmf3PickRayDirection);
		//모델 좌표계의 광선과 메쉬의 교차를 검사한다.
		nIntersected = m_pMesh->CheckRayIntersection(xmf3PickRayOrigin,
			xmf3PickRayDirection, pfHitDistance);
	}
	return(nIntersected);
}



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
XMFLOAT3 CExplosiveObject::m_pxmf3SphereVectors[EXPLOSION_DEBRISES];
CMesh* CExplosiveObject::m_pExplosionMesh = NULL;

CExplosiveObject::CExplosiveObject(ID3D12Device* pd3dDevice,ID3D12GraphicsCommandList* pd3dCommandList)
{
}

CExplosiveObject::~CExplosiveObject()
{
}

void CExplosiveObject::PrepareExplosion(ID3D12Device* pd3dDevice,
	ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (int i = 0; i < EXPLOSION_DEBRISES; i++) XMStoreFloat3(&m_pxmf3SphereVectors[i], ::RandomUnitVectorOnSphere());

	m_pExplosionMesh = new CSphereMeshDiffused(pd3dDevice, pd3dCommandList,1.0f, 20, 20);

}

void CExplosiveObject::Animate(float fElapsedTime)
{
	if (m_bBlowingUp)
	{
		m_fElapsedTimes += fElapsedTime;
		if (m_fElapsedTimes <= m_fDuration)
		{
			XMFLOAT3 xmf3Position = GetPosition();
			for (int i = 0; i < EXPLOSION_DEBRISES; i++)
			{
				m_pxmf4x4Transforms[i] = Matrix4x4::Identity();
				m_pxmf4x4Transforms[i]._41 = xmf3Position.x + m_pxmf3SphereVectors[i].x * m_fExplosionSpeed * m_fElapsedTimes;
				m_pxmf4x4Transforms[i]._42 = xmf3Position.y + m_pxmf3SphereVectors[i].y * m_fExplosionSpeed * m_fElapsedTimes;
				m_pxmf4x4Transforms[i]._43 = xmf3Position.z + m_pxmf3SphereVectors[i].z * m_fExplosionSpeed * m_fElapsedTimes;
				m_pxmf4x4Transforms[i] = Matrix4x4::Multiply(Matrix4x4::RotationAxis(m_pxmf3SphereVectors[i], m_fExplosionRotation * m_fElapsedTimes), m_pxmf4x4Transforms[i]);
			}
		}
		else
		{
			m_fElapsedTimes = 0;
			m_bBlowingUp = false;
		}
	}
	else
	{
		
		CRotatingObject::Animate(fElapsedTime);
	}
}

void CExplosiveObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_bBlowingUp)
	{
		for (int i = 0; i < EXPLOSION_DEBRISES; i++)
		{
			CGameObject::Render(pd3dCommandList, pCamera, &m_pxmf4x4Transforms[i], m_pExplosionMesh);
		}
	}
	else
	{
		CRotatingObject::Render(pd3dCommandList,pCamera);

	}
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, XMFLOAT4X4* pxmf4x4World, CMesh* pMesh)
{
	XMFLOAT4X4 xmmtx4x4TransposedWorld = Matrix4x4::Transpose(*pxmf4x4World);

	pd3dCommandList->SetGraphicsRoot32BitConstants(0, 16, &xmmtx4x4TransposedWorld, 0);

	if (pMesh)
	{
		pMesh->Render(pd3dCommandList);
	}
}


CWallObject::CWallObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

CWallObject::~CWallObject()
{
}

void CWallObject::Animate(float fTimeElapsed)
{
	CGameObject::Animate(fTimeElapsed);

	UpdateBoundingBox();

}
void CWallObject::UpdateBoundingBox()
{
	if (m_pMesh)
	{
		m_pMesh->m_xmOOBB.Transform(m_xmOOBB, XMLoadFloat4x4(&m_xmf4x4World));
	}
}

void CGameObject::TargetRotate()
{
	XMFLOAT3 LockedObjectPosition = m_pLockedObject;

	XMFLOAT3 xmf3Look = Vector3::Normalize(Vector3::Subtract(LockedObjectPosition, GetPosition()));
	XMFLOAT3 xmf3Right = Vector3::CrossProduct(XMFLOAT3(0.0f,1.0f,0.0f), xmf3Look, true);
	XMFLOAT3 xmf3Up = Vector3::CrossProduct(xmf3Look, xmf3Right, true);

	m_xmf4x4World._11 = xmf3Right.x; m_xmf4x4World._12 = xmf3Right.y; m_xmf4x4World._13 = xmf3Right.z;
	m_xmf4x4World._21 = xmf3Look.x; m_xmf4x4World._22 = xmf3Look.y; m_xmf4x4World._23 = xmf3Look.z;
	m_xmf4x4World._31 = xmf3Up.x; m_xmf4x4World._32 = xmf3Up.y; m_xmf4x4World._33 = xmf3Up.z;
}




CDuckObject::CDuckObject()
{
	XMStoreFloat4x4(&m_xmf4x4Transform, XMMatrixIdentity());
}

CDuckObject::~CDuckObject()
{
	if (m_pSiblingObject) delete m_pSiblingObject;
	if (m_pChildObject) delete m_pChildObject;
}

void CDuckObject::UpdateTransform(XMFLOAT4X4* pxmf4x4Parent)
{
	m_xmf4x4World = m_xmf4x4Transform;
	if (pxmf4x4Parent) m_xmf4x4World = Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent);

	if (m_pSiblingObject) m_pSiblingObject->UpdateTransform(pxmf4x4Parent);
	if (m_pChildObject) m_pChildObject->UpdateTransform(&m_xmf4x4World);
}

void CDuckObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CGameObject::Render(pd3dCommandList, pCamera);

	if (m_pSiblingObject) m_pSiblingObject->Render(pd3dCommandList, pCamera);
	if (m_pChildObject) m_pChildObject->Render(pd3dCommandList, pCamera);
}

CDuckObject* CDuckObject::FindFrame(LPSTR pstrFrameName)
{
	CDuckObject* pHierarchyObject = NULL;

	if (m_pstrFrameName == pstrFrameName) return this;

	if (m_pSiblingObject) if (pHierarchyObject = m_pSiblingObject->FindFrame(pstrFrameName)) return pHierarchyObject;
	if (m_pChildObject) if (pHierarchyObject = m_pChildObject->FindFrame(pstrFrameName)) return pHierarchyObject;

	return NULL;
}

void CDuckObject::SetPos(float x, float y, float z)
{
	m_xmf4x4Transform._41 = x;
	m_xmf4x4Transform._42 = y;
	m_xmf4x4Transform._43 = z;
}

void CDuckObject::SetAngle(XMFLOAT3& f3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&f3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
}

void CDuckObject::SetRotaion(XMMATRIX& xmmtRotation)
{
	m_xmf4x4Transform = Matrix4x4::Multiply(xmmtRotation, m_xmf4x4Transform);
}

void CDuckObject::SetScale(XMFLOAT3& f3Scaling)
{
	XMMATRIX mtxScale = XMMatrixScaling(f3Scaling.x, f3Scaling.y, f3Scaling.z);
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxScale, m_xmf4x4Transform);
}

void CDuckObject::SetRUL(XMFLOAT3& f3Right, XMFLOAT3& f3Up, XMFLOAT3& f3Look)
{
	m_xmf4x4Transform._11 = f3Right.x; m_xmf4x4Transform._12 = f3Right.y; m_xmf4x4Transform._13 = f3Right.z;
	m_xmf4x4Transform._21 = f3Up.x; m_xmf4x4Transform._22 = f3Up.y; m_xmf4x4Transform._23 = f3Up.z;
	m_xmf4x4Transform._31 = f3Look.x; m_xmf4x4Transform._32 = f3Look.y; m_xmf4x4Transform._33 = f3Look.z;
}



