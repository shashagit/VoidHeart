#include <hollowpch.h>
#include "PhysicsSystem.h"
#include "Hollow/Components/Transform.h"
#include "Hollow/Components/Body.h"
#include "Hollow/Components/Collider.h"
#include "Hollow/Managers/FrameRateController.h"
#include "Hollow/Managers/InputManager.h"

namespace Hollow
{
	PhysicsSystem PhysicsSystem::instance;

	void PhysicsSystem::AddGameObject(GameObject* object)
	{
		if (CheckComponents<Collider>(object)) {
			// Collider Init
			Collider* pCol = object->GetComponent<Collider>();
			
			pCol->mpTr = static_cast<Transform*>(object->GetComponent<Transform>());
			pCol->mpBody = static_cast<Body*>(object->GetComponent<Body>());

			// TODO write inertia formula for spheres
			glm::mat3 inertia = glm::mat3(0.0f);
			if (pCol->mpShape->mType == AABB) {
				inertia[0][0] = pCol->mpBody->mMass / 12.0f * (pCol->mpTr->mScale.y * pCol->mpTr->mScale.y + pCol->mpTr->mScale.z * pCol->mpTr->mScale.z);
				inertia[1][1] = pCol->mpBody->mMass / 12.0f * (pCol->mpTr->mScale.x * pCol->mpTr->mScale.x + pCol->mpTr->mScale.z * pCol->mpTr->mScale.z);
				inertia[2][2] = pCol->mpBody->mMass / 12.0f * (pCol->mpTr->mScale.y * pCol->mpTr->mScale.y + pCol->mpTr->mScale.x * pCol->mpTr->mScale.x);
			}
			if (pCol->mpBody->mInverseMass == 0.0f)
				pCol->mpBody->mLocalInertiaInverse = glm::mat3(0);
			else
				pCol->mpBody->mLocalInertiaInverse = glm::inverse(inertia);
			
			
			pCol->mpBody->mPosition = pCol->mpTr->mPosition;
			pCol->mpTr->mQuaternion = pCol->mpTr->mQuaternion;

			// update local shape
			static_cast<ShapeAABB*>(pCol->mpLocalShape)->mMin = glm::vec3(-0.5f, -0.5f, -0.5f) * (pCol->mpTr->mScale) + pCol->mpTr->mPosition;
			static_cast<ShapeAABB*>(pCol->mpLocalShape)->mMax = glm::vec3(0.5f, 0.5f, 0.5f) * (pCol->mpTr->mScale) + pCol->mpTr->mPosition;

			// Collider added to Dynamic BVH
			mTree.AddCollider(pCol);
		}
	}

	void PhysicsSystem::Step(float fixedDeltaTime)
	{
		for (unsigned int i = 0; i < mGameObjects.size(); ++i)
		{
			Collider* pCol = mGameObjects[i]->GetComponent<Collider>();
			glm::vec3 extents = static_cast<ShapeAABB*>(pCol->mpLocalShape)->GetHalfExtents();
			glm::vec3 x = glm::vec3(extents.x, 0.0f, 0.0f);
			glm::vec3 y = glm::vec3(0.0f, extents.y, 0.0f);
			glm::vec3 z = glm::vec3(0.0f, 0.0f, extents.z);
			glm::vec3 rotatedExtents = abs(glm::mat3(pCol->mpTr->mTransformationMatrix) * x) +
				abs(glm::mat3(pCol->mpTr->mTransformationMatrix) * y) +
				abs(glm::mat3(pCol->mpTr->mTransformationMatrix) * z);

			// based on normalized body vertices
			static_cast<ShapeAABB*>(pCol->mpShape)->mMin = glm::vec3(-rotatedExtents.x, -rotatedExtents.y, -rotatedExtents.z) + pCol->mpTr->mPosition;
			static_cast<ShapeAABB*>(pCol->mpShape)->mMax = glm::vec3(rotatedExtents.x, rotatedExtents.y, rotatedExtents.z) + pCol->mpTr->mPosition;
		}

		// balancing the tree
		mTree.Update();

		// finds out intersecting bounding boxes
		mTree.CalculatePairs();

		std::list < std::pair<Collider*, Collider*>>& pairs = mTree.GetPairs();

		for (auto& pair : pairs) {
			// perform the SAT intersection test
			mSAT.TestIntersection3D(pair.first, pair.second);
		}

		//HW_TRACE("{0}", mSAT.mContacts->size());

		for (auto go : mGameObjects)
		{
			Body* pBody = static_cast<Body*>(go->GetComponent<Body>());

			if (pBody->mInverseMass == 0.0f)
				continue;

			// compute acceleration
			glm::vec3 acc = pBody->mTotalForce * pBody->mInverseMass;
			acc += gravity;
			glm::vec3 alpha = pBody->mTotalTorque * pBody->mWorldInertiaInverse;

			// integrate acc into the velocity
			pBody->mVelocity += acc * fixedDeltaTime;
			pBody->mAngularVelocity += alpha * fixedDeltaTime;

			// set forces to zero
			pBody->mTotalForce = glm::vec3(0);
			pBody->mTotalTorque = glm::vec3(0);
		}

		// TODO remove loops and replace with hashmap
		if (!mSAT.mPrevContacts->empty())
		{
			for (auto itOld : *mSAT.mPrevContacts)
			{
				for (auto itNew : *mSAT.mContacts)
				{
					if (((*itNew).bodyA == (*itOld).bodyA &&
						(*itNew).bodyB == (*itOld).bodyB)) {

						// iterate through contact points
						for (int j = 0; j < (*itOld).contactPoints.size(); ++j)
						{
							Contact& cOld = (*itOld).contactPoints[j];

							for (int k = 0; k < (*itNew).contactPoints.size(); ++k) {
								Contact& cNew = (*itNew).contactPoints[k];

								if (glm::distance2(cNew.point, cOld.point) < proximityEpsilon)
								{
									cNew.normalImpulseSum = cOld.normalImpulseSum * bias;
									cNew.tangentImpulseSum1 = cOld.tangentImpulseSum1 * bias;
									cNew.tangentImpulseSum2 = cOld.tangentImpulseSum2 * bias;

									// apply old impulse as warm start
									(itNew)->constraint.EvaluateJacobian(&cNew, itNew->collisionNormal);
									itNew->constraint.ApplyImpulse(itNew->bodyA, itNew->bodyB, cNew.normalImpulseSum);

									itNew->constraint.EvaluateJacobian(&cNew, itNew->t0);
									itNew->constraint.ApplyImpulse(itNew->bodyA, itNew->bodyB, cNew.tangentImpulseSum1);

									itNew->constraint.EvaluateJacobian(&cNew, itNew->t1);
									itNew->constraint.ApplyImpulse(itNew->bodyA, itNew->bodyB, cNew.tangentImpulseSum2);
								}
							}

						}

					}
				}
			}
		}

		for (int i = 0; i < impulseIterations; ++i) {
			for (auto c : *mSAT.mContacts) {

				//std::cout << c->contactPoints[0]->normalImpulseSum << std::endl;
				int pointCount = c->contactPoints.size();

				for (int j = 0; j < pointCount; ++j) {

					c->constraint.EvaluateVelocityVector(c->bodyA, c->bodyB);

					//===== solve for normal constraint
					c->constraint.EvaluateJacobian(&c->contactPoints[j], c->collisionNormal);

					float effMass = 1.0f / (c->constraint.jacobian * c->constraint.massMatrixInverse * c->constraint.jacobianT);

					// bias value
					float b = baumgarte / fixedDeltaTime * std::min(c->contactPoints[j].penetrationDepth - slop, 0.0f);
					//float b = 0.1f / fixedDeltaTime * c->contactPoints[j]->penetrationDepth;

					float lambda = -effMass * (c->constraint.jacobian * c->constraint.velocityMatrix + b);
					float origNormalImpulseSum = c->contactPoints[j].normalImpulseSum;

					c->contactPoints[j].normalImpulseSum += lambda;
					c->contactPoints[j].normalImpulseSum =
						glm::clamp(c->contactPoints[j].normalImpulseSum, 0.0f, std::numeric_limits<float>::infinity());

					float deltaLambda = c->contactPoints[j].normalImpulseSum - origNormalImpulseSum;

					c->constraint.ApplyImpulse(c->bodyA, c->bodyB, deltaLambda);

					if (isApplyFriction) {
						//float nLambda = c->contactPoints[j]->normalImpulseSum;
						float nLambda = -gravity.y / pointCount;

						// calculate tangents (Erin Catto's code)
						glm::vec3 t0, t1;

						if (abs(c->collisionNormal.x) >= 0.57735f)
							t0 = glm::normalize(glm::vec3(c->collisionNormal.y, -c->collisionNormal.x, 0.0f));
						else
							t0 = glm::normalize(glm::vec3(0.0f, c->collisionNormal.z, -c->collisionNormal.y));
						t1 = glm::cross(c->collisionNormal, t0);

						//==== solve for tangent 0
						c->constraint.EvaluateJacobian(&c->contactPoints[j], t0);

						effMass = 1.0f / (c->constraint.jacobian * c->constraint.massMatrixInverse * c->constraint.jacobianT);

						lambda = -effMass * (c->constraint.jacobian * c->constraint.velocityMatrix + 0.0f);

						float origTangent0ImpulseSum = c->contactPoints[j].tangentImpulseSum1;

						c->contactPoints[j].tangentImpulseSum1 += lambda;
						c->contactPoints[j].tangentImpulseSum1 =
							glm::clamp(c->contactPoints[j].tangentImpulseSum1, -mu * nLambda, mu * nLambda);

						deltaLambda = c->contactPoints[j].tangentImpulseSum1 - origTangent0ImpulseSum;

						c->constraint.ApplyImpulse(c->bodyA, c->bodyB, deltaLambda);

						//==== solve for tangent 1
						c->constraint.EvaluateJacobian(&c->contactPoints[j], t1);

						effMass = 1.0f / (c->constraint.jacobian * c->constraint.massMatrixInverse * c->constraint.jacobianT);

						lambda = -effMass * (c->constraint.jacobian * c->constraint.velocityMatrix + 0.0f);
						float origTangent1ImpulseSum = c->contactPoints[j].tangentImpulseSum2;

						c->contactPoints[j].tangentImpulseSum2 += lambda;
						c->contactPoints[j].tangentImpulseSum2 =
							glm::clamp(c->contactPoints[j].tangentImpulseSum2, -mu * nLambda, mu * nLambda);

						deltaLambda = c->contactPoints[j].tangentImpulseSum2 - origTangent1ImpulseSum;

						c->constraint.ApplyImpulse(c->bodyA, c->bodyB, deltaLambda);
					}
				}
			}
		}

		mSAT.CopyContacts();
		
		for (auto go : mGameObjects)
		{
			Body* pBody = static_cast<Body*>(go->GetComponent<Body>());
			// save current position
			pBody->mPreviousPosition = pBody->mPosition;

			// integrate the position
			pBody->mPosition += pBody->mVelocity * fixedDeltaTime;
			
			// integrate the orientation
			glm::fquat newQuat = 0.5f * (pBody->mAngularVelocity) * pBody->mQuaternion * fixedDeltaTime;
			pBody->mQuaternion *= newQuat;
		}
	}

	void PhysicsSystem::InterpolateState(float blendingFactor)
	{
		for (auto go : mGameObjects)
		{
			Body* pBody = static_cast<Body*>(go->GetComponent<Body>());
			Transform* pTr = static_cast<Transform*>(go->GetComponent<Transform>());

			pTr->mPosition.x = pBody->mPosition.x * blendingFactor + pBody->mPreviousPosition.x * (1 - blendingFactor);
			pTr->mPosition.y = pBody->mPosition.y * blendingFactor + pBody->mPreviousPosition.y * (1 - blendingFactor);
			pTr->mPosition.z = pBody->mPosition.z * blendingFactor + pBody->mPreviousPosition.z * (1 - blendingFactor);

			//TODO setup slerp for quaternion interpolation while rendering

			pBody->mQuaternion = glm::normalize(pBody->mQuaternion);
			pBody->mRotationMatrix = glm::toMat3(pBody->mQuaternion);
			pBody->mWorldInertiaInverse =
				pBody->mRotationMatrix *
				pBody->mLocalInertiaInverse *
				glm::transpose(pBody->mRotationMatrix);

			//pTr->mRotation = pBody->mRotationMatrix;
		}
	}
	
	void PhysicsSystem::Update()
	{
		isPaused = InputManager::Instance().IsKeyTriggered(SDL_SCANCODE_P) == true ? !isPaused : isPaused;
		nextStep = InputManager::Instance().IsKeyTriggered(SDL_SCANCODE_SPACE);

		//================Physics Update======================
		float dt = FrameRateController::Instance().GetFrameTime();


		if (!isPaused) {
			accumulator += dt;
			while (accumulator > maxPossibleDeltaTime) {
				//{
					//Timer t;
				Step(maxPossibleDeltaTime);
				//}
				accumulator -= maxPossibleDeltaTime;
			}
		}
		else if (nextStep) {
			Step(maxPossibleDeltaTime);
		}
		InterpolateState(accumulator / maxPossibleDeltaTime);
		
	}
}
