#pragma once

#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class Scan : public Nodes::Base {
			public:
				Scan();
				string getTypeName() const override;
				void init();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void scan();
				void triangulate();
			protected:
				glm::mat4 lastCameraTransform;

				struct : ofParameterGroup {
					ofParameter<bool> useExistingData{ "Use existing data", false };
					
					struct : ofParameterGroup {
						ofParameter<float> maxResidual{ "Maximum residual", 0.05 };
						PARAM_DECLARE("Triangulate", maxResidual);
					} triangulate;

					PARAM_DECLARE("Scan", useExistingData, triangulate);
				} parameters;
			};
		}
	}
}