#pragma once

#include "Line.h"
#include "Transform.h"
#include "Camera.h"
#include "Ray.h"

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct LaserProjector_ {
			Transform_<T> rigidBodyTransform;
			glm::tvec2<T> fov;

			glm::tvec2<T> projectionPointToAngles(const glm::tvec2<T>& projectionPoint)
			{
				return {
					projectionPoint.x * this->fov.x / (T)2 * DEG_TO_RAD
					, projectionPoint.y * this->fov.y / (T)2 * DEG_TO_RAD
				};
			}

			Ray_<T> castRayObjectSpace(const glm::tvec2<T>& projectionPoint)
			{
				auto angles = this->projectionPointToAngles(projectionPoint);
				auto direction = glm::rotateX(glm::tvec3<T>(0, 0, 1), angles.y);
				direction = glm::rotateY(direction, angles.x);
				return Ray_<T> {
					glm::tvec3<T>()
					, direction
				};
			}

			Ray_<T> castRayWorldSpace(const glm::tvec2<T>& projectionPoint)
			{
				const auto rayObjectSpace = castRayObjectSpace(projectionPoint);
				const auto projectorPosition = this->rigidBodyTransform.translation;
				const auto rayTransmission = this->rigidBodyTransform.applyTransform(rayObjectSpace.t) - projectorPosition;
				return Ray_<T> {
					projectorPosition
					, rayTransmission
				};
			}

			Line_<T> projectIntoCamera(const glm::tvec2<T>& projectionPoint
				, const Camera_<T>& camera)
			{
				
			}

			template<typename T2>
			LaserProjector_<T2> castTo() const
			{
				return LaserProjector_<T2> {
					this->rigidBodyTransform.castTo<T2>
					, (glm::tvec2<T2>) this->fov
				};
			}
		};

		typedef LaserProjector_<float> LaserProjector;
	}
}