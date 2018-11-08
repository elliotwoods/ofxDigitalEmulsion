#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include <aruco/aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS MarkerMap : public Nodes::Base {
			public:
				MarkerMap();
				string getTypeName() const override;
				void init();
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<aruco::MarkerMap> getMarkerMap();

				void clear();
				void rotateMarkerMap(const ofVec3f & axis, float angle);
				void removeMarker(size_t idToRemove);
			protected:
				shared_ptr<aruco::MarkerMap> markerMap;

				struct : ofParameterGroup {
					ofParameter<WhenDrawOnWorldStage> drawLabels{ "Draw labels", WhenDrawOnWorldStage::Selected };
					PARAM_DECLARE("MarkerMap", drawLabels);
				} parameters;
			};
		}
	}
}