#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include <opencv2/aruco/dictionary.hpp>
#include <opencv2/aruco/charuco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS ChArUcoBoard : public Nodes::Item::AbstractBoard {
			public:
				struct PaperSize {
					string name;
					float width;
					float height;
				};

				ChArUcoBoard();
				string getTypeName() const override;
				void init();
				void update();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				bool findBoard(cv::Mat, vector<cv::Point2f> & imagePoints, vector<cv::Point3f> & objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const override;
				void drawObject() const override;
				float getSpacing() const override; 

				ofxCvGui::PanelPtr getPanel() override;

			protected:
				float getPreviewPixelsPerMeter() const;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<int> width{ "Width", 11 };
						ofParameter<int> height{ "Height", 8 };
						PARAM_DECLARE("Size [squares]", width, height);
					} size;

					struct : ofParameterGroup {
						ofParameter<float> square{ "Square", 0.025, 0.001, 0.10 };
						ofParameter<float> marker{ "Marker", 0.0175, 0.001, 0.1 };
						PARAM_DECLARE("Length [m]", square, marker);
					} length;
					

					struct : ofParameterGroup {
						ofParameter<bool> refineStrategy{ "Refine strategy", true };
						ofParameter<float> errorCorrectionRate{ "Error correction rate", 0.6 };
						PARAM_DECLARE("Detection", refineStrategy, errorCorrectionRate);
					} detection;

					struct : ofParameterGroup {
						ofParameter<int> DPI{ "Preview DPI", 75 };
						ofParameter<bool> showPaperSizes{ "Show paper sizes", false };
						PARAM_DECLARE("Preview", DPI, showPaperSizes);
					} preview;

					PARAM_DECLARE("ChAruCoBoard", size, length, detection, preview);
				} parameters;

				cv::Ptr<cv::aruco::Dictionary> dictionary;
				cv::Ptr<cv::aruco::CharucoBoard> board;
				float cachedDPI = 0;

				ofImage preview;
				ofxCvGui::PanelPtr panel;

				set<shared_ptr<PaperSize>> paperSizes;
			};
		}
	}
}