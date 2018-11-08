#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Heliostats : public Nodes::Base {
				public:
					class Heliostat : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Heliostat();
						string getDisplayString() const override;
						void drawWorld();

						ofParameter<string> name{ "Name", "" };
						ofParameter<ofVec3f> position{ "Position", ofVec3f() };
						ofParameter<float> rotationY { "Rotation Y", 0.0f, -90, 90 };

						ofParameter<int> axis1Servo{ "Axis 1 Servo", 0 };
						ofParameter<int> axis2Servo{ "Axis 2 Servo", 0 };

						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						Utils::CaptureSet<BoardOnMirror::Capture> captures;
						void calcPosition(float axis2ToPlaneLength = 0.15f);

					protected:
						ofxCvGui::ElementPtr getDataDisplay() override;
					};

					Heliostats();
					string getTypeName() const override;

					void init();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					ofxCvGui::PanelPtr getPanel() override;
					void drawWorldStage();

					vector<shared_ptr<Heliostat>> getHeliostats();
					void add(shared_ptr<Heliostat>);
					void removeHeliostat(shared_ptr<Heliostat> heliostat);
				protected:
					Utils::CaptureSet<Heliostat> heliostats;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;
				};
			}
		}
	}
}