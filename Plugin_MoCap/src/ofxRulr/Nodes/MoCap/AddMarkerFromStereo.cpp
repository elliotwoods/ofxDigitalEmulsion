#include "pch_Plugin_MoCap.h"
#include "AddMarkerFromStereo.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"
#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
#pragma mark RecordMarkerForTraining
			//----------
			AddMarkerFromStereo::AddMarkerFromStereo() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string AddMarkerFromStereo::getTypeName() const {
				return "MoCap::AddMarkerFromStereo";
			}

			//----------
			void AddMarkerFromStereo::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				auto findMarkerCentroidsInputA = this->addInput<FindMarkerCentroids>("FindMarkerCentroids A");
				findMarkerCentroidsInputA->onNewConnection += [this](shared_ptr<FindMarkerCentroids> node) {
					node->onNewFrame.addListener([this](shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
						this->incomingFramesA.send(incomingFrame);
					}, this);
				};
				findMarkerCentroidsInputA->onDeleteConnection += [this](shared_ptr<FindMarkerCentroids> node) {
					if (node) {
						node->onNewFrame.removeListeners(this);
					}
				};

				auto findMarkerCentroidsInputB = this->addInput<FindMarkerCentroids>("FindMarkerCentroids B");
				findMarkerCentroidsInputB->onNewConnection += [this](shared_ptr<FindMarkerCentroids> node) {
					node->onNewFrame.addListener([this](shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
						this->incomingFramesB.send(incomingFrame);
					}, this);
				};
				findMarkerCentroidsInputB->onDeleteConnection += [this](shared_ptr<FindMarkerCentroids> node) {
					if (node) {
						node->onNewFrame.removeListeners(this);
					}
				};


				this->addInput<Procedure::Calibrate::StereoCalibrate>();
				this->addInput<Body>();

				
				{
					auto panel = ofxCvGui::Panels::Groups::makeStrip();
					auto makeCameraPanel = [panel](ofImage & preview, shared_ptr<FindMarkerCentroidsFrame> & frame, unique_ptr<glm::vec2> & centroidStore) {
						auto panel = ofxCvGui::Panels::makeImage(preview);
						auto panelWeak = weak_ptr<ofxCvGui::Panels::Image>(panel);
						panel->onDrawImage += [&centroidStore] (ofxCvGui::DrawImageArguments & args) {
							if (centroidStore) {
								ofPushStyle();
								{
									ofSetColor(255, 0, 0);
									ofPushMatrix();
									{
										ofTranslate(*centroidStore);
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
									}
									ofPopMatrix();
								}
								ofPopStyle();
							}
						};
						panel->onMouseReleased += [& centroidStore, panelWeak, &frame](ofxCvGui::MouseArguments & args) {
							auto panel = panelWeak.lock();
							auto coordInCameraFrame = Utils::applyTransform(
								glm::inverse(panel->getPanelToImageTransform())
								, {
									args.local.x
									, args.local.y
									, 0.0f
								});

							//search for centroid
							auto count = frame->centroids.size();
							for (int i = 0; i < count; i++) {
								if (frame->boundingRects[i].contains(ofxCv::toCv((glm::vec2) coordInCameraFrame))) {
									centroidStore = make_unique<glm::vec2>(ofxCv::toOf(frame->centroids[i]));
								}
							}
						};
						return panel;
					};
					panel->add(makeCameraPanel(this->previewA, this->frameA, this->centroidA));
					panel->add(makeCameraPanel(this->previewB, this->frameB, this->centroidB));
					this->panel = panel;
				}

				this->panel = panel;
			}

			//----------
			void AddMarkerFromStereo::update() {
				{
					{
						bool receivedFrame = false;
						while (this->incomingFramesA.tryReceive(this->frameA)) {
							receivedFrame = true;
						};
						if (receivedFrame) {
							ofxCv::copy(this->frameA->difference, this->previewA.getPixels());
							this->previewA.update();
						}
					}

					{
						bool receivedFrame = false;
						while (this->incomingFramesB.tryReceive(this->frameB)) {
							receivedFrame = true;
						};
						if (receivedFrame) {
							ofxCv::copy(this->frameB->difference, this->previewB.getPixels());
							this->previewB.update();
						}
					}
				}

				{
					if (this->centroidA && this->centroidB) {
						auto stereoCalibrateNode = this->getInput<Procedure::Calibrate::StereoCalibrate>();
						if (stereoCalibrateNode) {
							vector<glm::vec2> imagePointsA(1, *this->centroidA);
							vector<glm::vec2> imagePointsB(1, *this->centroidB);
							auto worldPositions = stereoCalibrateNode->triangulate(imagePointsA, imagePointsB, false);
							this->estimatedPosition = make_unique<glm::vec3>(worldPositions[0]);
						}
					}
					else {
						this->estimatedPosition.reset();
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr AddMarkerFromStereo::getPanel() {
				return this->panel;
			}

			//----------
			void AddMarkerFromStereo::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void AddMarkerFromStereo::deserialize(const nlohmann::json& json) {
				Utils::deserialize(json, this->parameters);
			}

			//----------
			void AddMarkerFromStereo::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);

				inspector->addLiveValue<glm::vec3>("Estimated position", [this]() {
					if (this->estimatedPosition) {
						return *this->estimatedPosition;
					}
					else {
						return glm::vec3();
					}
				});
				inspector->addButton("Clear estimation", [this]() {
					this->estimatedPosition.reset();
				});
				inspector->addButton("Add marker", [this]() {
					try {
						this->addMarker();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			glm::vec3 AddMarkerFromStereo::getMarkerPositionEstimation() const {
				if (this->estimatedPosition) {
					return * this->estimatedPosition;
				}
				else {
					throw(ofxRulr::Exception("You must select a centroid in both images to find marker"));
				}
			}

			//----------
			void AddMarkerFromStereo::drawWorldStage() {
				try {
					auto position = this->getMarkerPositionEstimation();

					ofPushStyle();
					{
						ofColor color(200, 100, 100);
						color.setHueAngle(fmod(ofGetElapsedTimef() * 360.0f, 360.0f));
						ofSetColor(color);
						ofDrawSphere(position, this->parameters.previewRadius);
					}
					ofPopStyle();
				}
				catch (...) {

				}
			}

			//----------
			void AddMarkerFromStereo::addMarker() {
				this->throwIfMissingAConnection<Body>();
				auto body = this->getInput<Body>();
				body->addMarker(Utils::applyTransform(glm::inverse(body->getTransform()), this->getMarkerPositionEstimation()));

				this->centroidA.reset();
				this->centroidB.reset();
				this->estimatedPosition.reset();
			}

		}
	}
}