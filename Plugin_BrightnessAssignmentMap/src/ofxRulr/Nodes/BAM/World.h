#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			class Projector;
			
			class World : public Nodes::Base {
			public:
				World();
				string getTypeName() const override;
				void init();
				void populateInspector(ofxCvGui::InspectArguments &);
				void registerProjector(shared_ptr<Projector>);
				void unregisterProjector(shared_ptr<Projector>);
				vector<shared_ptr<Projector>> getProjectors() const;
				const ofParameterGroup & getParameters() const;
				void exportAll(const string & folderPath) const;
			protected:
				vector<weak_ptr<Projector>> projectors;

				struct : ofParameterGroup {
					ofParameter<float> normalCutoffAngle{ "Normal cutoff angle (�)", 80.0, 0.0, 90.0 };
					ofParameter<float> featherSize{ "Feather size", 0.2, 0.0, 1.0 };
					ofParameter<float> targetBrightness{ "Target brightness", 0.8f };
					PARAM_DECLARE("World", normalCutoffAngle, featherSize, targetBrightness);
				} parameters;
			};
		}
	}
}