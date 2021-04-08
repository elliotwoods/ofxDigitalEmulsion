#pragma once

#include "View.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Projector : public View {
			public:
				Projector();
				void init();
				string getTypeName() const;

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				ofxRay::Projector projector;
			};
		}
	}
}