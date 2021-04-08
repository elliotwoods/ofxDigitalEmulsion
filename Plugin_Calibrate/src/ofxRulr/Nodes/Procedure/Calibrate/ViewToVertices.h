#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/IReferenceVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ViewToVertices : public Base {
				public:
					class ViewArea : public ofBaseDraws {
					public:
						ViewArea(ViewToVertices &);
						void draw(float x, float y) const override;
						void draw(float x, float y, float w, float h) const override;
						float getHeight() const override;
						float getWidth() const override;
					protected:
						ViewToVertices & parent;
					};
					ViewToVertices();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorldStage();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void calibrate(); // will throw on fail
				protected:
					void drawOnProjector();

					ViewArea viewArea;
					ofxCvGui::PanelPtr view;

					ofParameter<filesystem::path> projectorReferenceImageFilename;
					ofImage projectorReferenceImage;
					ofParameter<bool> dragVerticesEnabled;
					ofParameter<bool> calibrateOnVertexChange;
					ofParameter<bool> useExistingParametersAsInitial;
					bool success;
					float reprojectionError;
					weak_ptr<IReferenceVertices::Vertex> selection;
				};
			}
		}
	}
}
