#pragma once

#include "ofMain.h"

namespace ofxRulr {
	namespace Data {
		namespace Dosirak {
			struct Curve {
				ofFloatColor color;
				vector<glm::vec3> points;
				bool closed;

				void updatePreview();
				void drawPreview() const;

				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);
			protected:
				ofPolyline preview;
			};

			struct Curves : map<string, Data::Dosirak::Curve> {
				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);
			};
		}
	}
}