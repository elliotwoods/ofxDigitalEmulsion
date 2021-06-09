#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Halo : public Item::RigidBody {
				public:
					Halo();
					string getTypeName() const override;

					void init();

					void drawObject();
				protected:
					struct : ofParameterGroup {
						ofParameter<float> diameter{ "Diameter", 2.0f, 0.0f, 10.0f };

						struct : ofParameterGroup {
							ofParameter<bool> asPath{ "As path", true };
							ofParameter<float> thickness{ "Thickness", 0.1f, 0.0f, 1.0f };
							ofParameter<int> resolution{ "Resolution", 100 };
							PARAM_DECLARE("Draw", asPath, thickness, resolution);
						} draw;
						PARAM_DECLARE("Halo", diameter, draw);
					} parameters;
				};
			}
		}
	}
}