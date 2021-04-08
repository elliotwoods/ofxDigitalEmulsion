#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Widgets.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			class openFrameworks : public Base {
			public:
				openFrameworks();
				string getTypeName() const override;

				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);
				//void serialize(nlohmann::json &);
				//void deserialize(const nlohmann::json &);

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				ofxCvGui::PanelPtr panel;
				ofImage * image;
				bool vsync;
			};
		}
	}
}