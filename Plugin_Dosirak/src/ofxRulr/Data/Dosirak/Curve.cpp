#include "pch_Plugin_Dosirak.h"
#include "Curve.h"

namespace ofxRulr {
	namespace Data {
		namespace Dosirak {
#pragma mark Curve
			//----------
			void
				Curve::updatePreview()
			{
				this->preview.clear();
				this->preview.addVertices(this->points);
				if (this->closed) {
					this->preview.close();
				}
			}

			//----------
			void
				Curve::drawPreview() const
			{
				ofPushStyle();
				{
					ofSetColor(this->color);
					if (this->points.size() == 0) {

					}
					else if (this->points.size() == 1) {
						ofDrawSphere(this->points.front(), 0.1f);
					}
					else {
						this->preview.draw();
					}
				}
				ofPopStyle();
			}

			//----------
			void
				Curve::serialize(nlohmann::json& json) const
			{
				{
					auto& jsonColor = json["color"];
					for (int i = 0; i < 4; i++) {
						jsonColor[i] = this->color[i];
					}
				}

				{
					auto& jsonPoints = json["points"];

					for (const auto& point : this->points) {
						nlohmann::json jsonPoint;
						for (int i = 0; i < 3; i++) {
							jsonPoint[i] = point[i];
						}
						jsonPoints.push_back(jsonPoint);
					}
				}
			}

			//----------
			void
				Curve::deserialize(const nlohmann::json& json)
			{
				if (json.contains("color")) {
					const auto& jsonColor = json["color"];
					if (jsonColor.size() >= 4) {
						for (int i = 0; i < 4; i++) {
							this->color[i] = jsonColor[i];
						}
					}
				}

				if (json.contains("points")) {
					this->points.clear();

					const auto& jsonPoints = json["points"];
					for (const auto& jsonPoint : jsonPoints) {
						glm::vec3 point;
						if (jsonPoint.size() >= 3) {
							for (int i = 0; i < 3; i++) {
								point[i] = jsonPoint[i];
							}
						}
						this->points.push_back(point);
					}
				}
			}

			//----------
			Curve
				Curve::getTransformed(const glm::mat4& transform) const
			{
				Curve transformedCurve;
				{
					transformedCurve.color = this->color;
					transformedCurve.closed = this->closed;
				}

				transformedCurve.points.reserve(this->points.size());

				for (const auto& it : this->points) {
					auto transformedPoint = ofxCeres::VectorMath::applyTransform(transform, it);
					transformedCurve.points.push_back(transformedPoint);
				}

				return transformedCurve;
			}

			//----------
			bool
				Curve::operator!=(const Curve & other) const
			{
				// being lazy to install vvvv for missing symbol so writing code here
				for (int i = 0; i < 4; i++) {
					if (other.color[i] != this->color[i]) {
						//return true; // <-- we ignore color changes for now because lissajous track colour is always changing HACK
					}
				}

				if (other.points.size() != this->points.size()) {
					return true;
				}
				for (int i = 0; i < other.points.size(); i++) {
					if (other.points[i] != this->points[i]) {
						return true;
					}
				}
				return false;
			}

#pragma mark Curves
			//----------
			void
				Curves::serialize(nlohmann::json& json) const
			{
				for (const auto it : *this) {
					const auto& curveName = it.first;
					const auto& curve = it.second;

					nlohmann::json jsonCurve;
					curve.serialize(jsonCurve);

					json[curveName] = jsonCurve;
				}
			}

			//----------
			void
				Curves::deserialize(const nlohmann::json& json)
			{
				this->clear();

				for (auto it = json.begin(); it != json.end(); ++it) {
					const auto& curveName = it.key();
					const auto& jsonCurve = it.value();
					
					// Add a blank curve with this name to this
					auto it2 = this->emplace(it.key(), Curve());
					this->at(it.key()).deserialize(jsonCurve);
				}
			}

			//----------
			Curves
				Curves::getTransformed(const glm::mat4& transform) const
			{
				Curves transformedCurves;
				for (const auto& it : *this) {
					transformedCurves.emplace(it.first, it.second.getTransformed(transform));
				}
				return transformedCurves;
			}

			//----------
			bool
				Curves::operator!=(const Curves& other) const
			{
				if (other.size() != this->size()) {
					return true;
				}

				for (const auto & it : other) {
					// check we have a curve with matching name
					auto findCurve = this->find(it.first);
					if (findCurve == this->end()) {
						return true;
					}

					// Check the curves match
					if (findCurve->second != it.second) {
						return true;
					}
				}

				return false;
			}

		}
	}
}