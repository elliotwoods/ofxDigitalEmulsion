#include "pch_Plugin_Calibrate.h"
#include "MovingHeadToWorld.h"

#include "ofxRulr/Nodes/DMX/MovingHead.h"
#include "ofxRulr/Nodes/DMX/Sharpy.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"


using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark MovingHeadToWorld::Model
				//---------
				MovingHeadToWorld::Model::Model() {

				}

				//---------
				MovingHeadToWorld::Model::Model(const glm::vec3 & initialPosition, const glm::vec3 & initialRotationEuler) {
					this->initialPosition = initialPosition;
					this->initialRotationEuler = initialRotationEuler;
				}

				//---------
				unsigned int MovingHeadToWorld::Model::getParameterCount() const {
					return 7;
				}

				//---------
				void MovingHeadToWorld::Model::resetParameters() {
					auto parameters = this->getParameters();
					
					//first check for nan's
					bool foundNan = false;
					for (int i = 0; i < 3; i++) {
						if (isnan(this->initialPosition[i]) || isnan(this->initialRotationEuler[i])) {
							foundNan |= true;
							break;
						}
					}

					//if our input is valid then use it
					if (!foundNan) {
						*parameters++ = this->initialPosition[0];
						*parameters++ = this->initialPosition[1];
						*parameters++ = this->initialPosition[2];

						*parameters++ = this->initialRotationEuler[0];
						*parameters++ = this->initialRotationEuler[1];
						*parameters++ = this->initialRotationEuler[2];
					}
					else {
						//use zeros otherwise
						*parameters++ = 0.0;
						*parameters++ = 0.0;
						*parameters++ = 0.0;

						*parameters++ = 0.0;
						*parameters++ = 0.0;
						*parameters++ = 0.0;
					}
					
					*parameters++ = 0.0; // tiltOffset
				}

				//---------
				void MovingHeadToWorld::Model::getResidual(DataPoint point, double & residual, double * gradient) const {
					if (gradient) {
						RULR_ERROR << "ofxNonLinearFit::Models::RigidBody does not support gradient algorithms";
					}

					auto pointEvaluated = point;
					this->evaluate(pointEvaluated);
					auto difference = pointEvaluated.panTilt - point.panTilt;
					while (difference.x > 180.0f) {
						difference.x -= 360.0f;
					}
					while (difference.x < -180.0f) {
						difference.x += 360.0f;
					}
					residual = glm::length2(difference);
				}

				//---------
				void MovingHeadToWorld::Model::evaluate(DataPoint & point) const {
					auto pointInObjectSpace = Utils::applyTransform(glm::inverse(this->transform), point.world);
					point.panTilt = DMX::MovingHead::getPanTiltForTargetInObjectSpace(pointInObjectSpace, this->tiltOffset);
				}

				//---------
				void MovingHeadToWorld::Model::cacheModel() {
					if (this->isReady()) {
						auto parameters = this->getParameters();

						// WARNING : TODO : are these angles in degrees?
						auto position = glm::vec3(parameters[0], parameters[1], parameters[2]);
						auto rotationEuler = glm::vec3(parameters[3], parameters[4], parameters[5]);
						auto rotationQuat = glm::quat(rotationEuler);
						this->transform = (glm::mat4) (ofMatrix4x4::newRotationMatrix(rotationQuat) * ofMatrix4x4::newTranslationMatrix(position));

						this->tiltOffset = parameters[6];
					}
				}

				//---------
				const glm::mat4 & MovingHeadToWorld::Model::getTransform() {
					return this->transform;
				}

				//---------
				float MovingHeadToWorld::Model::getTiltOffset() const {
					return this->tiltOffset;
				}

#pragma mark MovingHeadToWorld
				//---------
				MovingHeadToWorld::MovingHeadToWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				void MovingHeadToWorld::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::RigidBody>("Marker");
					this->addInput<DMX::MovingHead>();

					auto view = make_shared<Panels::Base>();
					view->onDraw += [this](ofxCvGui::DrawArguments & args) {
						auto movingHead = this->getInput<DMX::MovingHead>();
						if (movingHead) {
							ofPushStyle();
							{
								auto viewWidth = args.localBounds.width;
								auto viewHeight = args.localBounds.height;

								ofSetLineWidth(1.0f);
								//center line
								ofDrawLine(viewWidth / 2.0f, 0.0f, viewWidth / 2.0f, viewHeight);
								//horizon line
								auto horizonHeight = ofMap(90.0f, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);
								ofDrawLine(0.0f, horizonHeight, viewWidth, horizonHeight);


								auto panTiltToView = [movingHead, viewWidth, viewHeight](const glm::vec2 & panTilt) {
									auto drawX = ofMap(panTilt.x, +180.0f, -180.0f, 0.0f, viewWidth);
									auto drawY = ofMap(panTilt.y, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);
									return glm::vec2(drawX, drawY);
								};

								ofFill();
								ofDrawCircle(panTiltToView(movingHead->getPanTilt()), 5.0f);
								ofNoFill();
								ofSetLineWidth(2.0f);
								for (const auto & dataPoint : this->dataPoints) {
									const auto positionInView = panTiltToView(dataPoint.panTilt);
									if (dataPoint.residual != 0) {
										ofDrawBitmapString(ofToString(dataPoint.residual, 3), positionInView);
										ofDrawLine(positionInView, panTiltToView(dataPoint.panTiltEvaluated));
									}
									ofDrawCircle(positionInView, 5.0f);
								}
							}
							ofPopStyle();
						}
					};
					auto viewWeak = weak_ptr<Panels::Base>(view);
					view->onMouse += [this, viewWeak](MouseArguments & args) {
						auto view = viewWeak.lock();
						if (view) {
							//take the mouse press (for drag later)
							args.takeMousePress(view);

							auto movingHead = this->getInput<DMX::MovingHead>();
							if (movingHead) {
								//if dragging
								if (args.isDragging(view)) {
									auto panTilt = movingHead->getPanTilt();
									auto movement = args.movement * (ofGetKeyPressed(OF_KEY_SHIFT) ? 0.01f : 0.2f);
									panTilt.x = ofClamp(panTilt.x - movement.x, -180.0f, +180.0f);
									panTilt.y = ofClamp(panTilt.y + movement.y, 0.0f, movingHead->getMaxTilt());
									movingHead->setPanTilt(panTilt);
								}
							}
						}
					};
					view->onKeyboard += [this, viewWeak](KeyboardArguments & args) {
						auto view = viewWeak.lock();
						if (view) {
							if (args.checkCurrentPanel(view.get())) {
								if (args.action == KeyboardArguments::Action::Pressed) {
									switch (args.key) {
									case 'a':
									{
										try {
											ofxRulr::Utils::ScopedProcess scopedProcess("MovingHeadToWorld - addCapture");
											this->addCapture();
											scopedProcess.end();
										}
										RULR_CATCH_ALL_TO_ALERT;
										break;
									}

									case 'd':
									{
										this->deleteLastCapture();
										break;
									}
										
									default:
										break;
									}
								}
							}
						}
					};
					this->view = view;

					this->lastFindTime = 0.0f;
					this->residual = 0.0f;
					this->beamBrightness.set("Beam brightness", 0.2f, 0.0f, 1.0f);
					this->calibrateOnAdd.set("Calibrate on add", true);
					this->continuouslyTrack.set("Continuously track", false);
				}

				//---------
				string MovingHeadToWorld::getTypeName() const {
					return "Procedure::Calibrate::MovingHeadToWorld";
				}

				//---------
				void MovingHeadToWorld::update() {
					auto movingHead = this->getInput<DMX::MovingHead>();
					if (movingHead) {
						//fade the brightness based on last find time
						float ageOfLastFind = ofGetElapsedTimef() - lastFindTime;
						movingHead->setBrightness(ofMap(ageOfLastFind, 0, 1.0f, 1.0f, this->beamBrightness, true));

						if (this->continuouslyTrack) {
							try {
								this->throwIfMissingAConnection<Item::RigidBody>();
								movingHead->lookAt(this->getInput<Item::RigidBody>()->getPosition());
							}
							RULR_CATCH_ALL_TO_ALERT;
						}
					}
				}

				//---------
				void MovingHeadToWorld::serialize(nlohmann::json & json) {
					Utils::serialize(json, this->beamBrightness);
					Utils::serialize(json, this->calibrateOnAdd);

					auto & jsonDataPoints = json["dataPoints"];
					for (int i = 0; i < this->dataPoints.size(); i++) {
						jsonDataPoints[i]["world"] << this->dataPoints[i].world;
						jsonDataPoints[i]["panTilt"] << this->dataPoints[i].panTilt;
						jsonDataPoints[i]["residual"] << this->dataPoints[i].residual;
						jsonDataPoints[i]["panTiltEvaluated"] << this->dataPoints[i].panTiltEvaluated;
					}
					json["residual"] << this->residual;
				}

				//---------
				void MovingHeadToWorld::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->beamBrightness);
					Utils::deserialize(json, this->calibrateOnAdd);
					
					this->dataPoints.clear();
					const auto & jsonDataPoints = json["dataPoints"];
					for (const auto & jsonDataPoint : jsonDataPoints) {
						DataPoint dataPoint;
						jsonDataPoint["world"] >> dataPoint.world;
						jsonDataPoint["panTilt"] >> dataPoint.panTilt;
						jsonDataPoint["residual"] >> dataPoint.residual;
						jsonDataPoint["panTiltEvaluated"] >> dataPoint.panTiltEvaluated;
						this->dataPoints.push_back(dataPoint);
					}
					json["residual"] >> this->residual;
				}

				//---------
				void MovingHeadToWorld::populateInspector(InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->add(new Widgets::Slider(this->beamBrightness));

					auto addCaptureButton = new Widgets::Button("Add capture", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("MovindHeadToWorld - addCapture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, ' ');
					addCaptureButton->setHeight(100.0f);
					inspector->add(addCaptureButton);

					inspector->add(new Widgets::LiveValue<size_t>("Data points", [this]() {
						return this->dataPoints.size();
					}));
					inspector->add(new Widgets::Button("Clear captures", [this]() {
						this->dataPoints.clear();
					}));
					inspector->add(new Widgets::Toggle(this->calibrateOnAdd));

					auto calibrateButton = new Widgets::Button("Calibrate", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("Calibrate");
							if (this->calibrate()) {
								scopedProcess.end();
							}
						}
						RULR_CATCH_ALL_TO_ALERT

					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);

					inspector->add(new Widgets::LiveValue<float>("Residual", [this]() {
						return this->residual;
					}));

					inspector->add(new Widgets::Title("Tracking", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::Button("Aim at target", [this]() {
							try {
								this->performAim();
							}
							RULR_CATCH_ALL_TO_ALERT;
						}, 't'));
						inspector->add(new Widgets::Toggle(this->continuouslyTrack));
					}

					inspector->add(new Widgets::Title("Aim beam", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::Button("Forwards", [this]() {
							this->setPanTiltOrAlert(glm::vec2(0, 90));
						}));
						inspector->add(new Widgets::Button("20 degree incline", [this]() {
							this->setPanTiltOrAlert(glm::vec2(0, 70));
						}));
						inspector->add(new Widgets::Button("Upwards", [this]() {
							this->setPanTiltOrAlert(glm::vec2(0, 0));
						}));
					}
				}

				//---------
				void MovingHeadToWorld::drawWorldStage() {
					ofMesh lines;
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto movingHeadRotation = movingHead ? movingHead->getRotationQuat() : ofQuaternion();

					for (const auto & dataPoint : this->dataPoints) {
						lines.addVertex(dataPoint.world);
						lines.addColor(ofColor(255, 100, 100, 255));
						auto vector = glm::vec3(0, 0.1f, 0.0f);
						auto rotationoF = (ofMatrix4x4)movingHeadRotation * ofQuaternion(dataPoint.panTilt.x, glm::vec3(0, -1, 0)) * ofQuaternion(dataPoint.panTilt.y, glm::vec3(1,0,0));
						auto rotation = (glm::quat)rotationoF;
						lines.addVertex(vector * rotation + dataPoint.world);
						lines.addColor(ofColor(255, 0));
					}
					lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
					lines.draw();
					lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					Utils::Graphics::pushPointSize(3.0f);
					{
						lines.draw(); //draw as dots also
					}
					Utils::Graphics::popPointSize();
				}

				//---------
				ofxCvGui::PanelPtr MovingHeadToWorld::getPanel() {
					return this->view;
				}

				//---------
				void MovingHeadToWorld::addCapture() {
					this->throwIfMissingAnyConnection();
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto target = this->getInput<Item::RigidBody>();

					if (target->getTransform() == glm::mat4(1.0f)) {
						//presume we didn't get any tracking if it's 0,0
						throw(Exception("Target has no transform, presuming bad tracking."));
					}
					DataPoint dataPoint = {
						target->getPosition(),
						movingHead->getPanTilt(),
						0.0f
					};

					this->dataPoints.push_back(dataPoint);
					this->lastFindTime = ofGetElapsedTimef();
				}

				//---------
				void MovingHeadToWorld::deleteLastCapture() {
					if (!this->dataPoints.empty()) {
						this->dataPoints.pop_back();
					}
				}

				//---------
				bool MovingHeadToWorld::calibrate(int iterations) {
					this->throwIfMissingAConnection<DMX::MovingHead>();
					auto movingHead = this->getInput<DMX::MovingHead>();

					bool valid = true;

					//perform calibrate multiple times (sometimes it takes more than once to get the result)
					for (int i = 0; i < 5; i++) {
						auto fit = ofxNonLinearFit::Fit<Model>();
						auto model = Model(movingHead->getPosition(), movingHead->getRotationEuler());

						double residual;
						fit.optimise(model, &this->dataPoints, &residual);

						//get out the result
						const auto & resultTransform = model.getTransform();
						const auto resultTiltOffset = model.getTiltOffset();

						//check if result is valid
						auto valid = !glm::any(
							glm::isnan(resultTransform[0])
							|| glm::isnan(resultTransform[1]) 
							|| glm::isnan(resultTransform[2]) 
							|| glm::isnan(resultTransform[3]))
							&& !isnan(resultTiltOffset);

						if (valid) {
							movingHead->setTransform(resultTransform);
							movingHead->setTiltOffset(resultTiltOffset);

							this->residual = residual;

							for (auto & dataPoint : this->dataPoints) {
								auto dataPointEvaluated = dataPoint;
								model.evaluate(dataPointEvaluated);
								double residual;
								model.getResidual(dataPoint, residual, nullptr);
								dataPoint.residual = residual;
								dataPoint.panTiltEvaluated = dataPointEvaluated.panTilt;
							}
						}
						else {
							valid = false;
							break;
						}
					}
					
					return valid;
				}

				//---------
				void MovingHeadToWorld::performAim() {
					this->throwIfMissingAnyConnection();
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto target = this->getInput<Item::RigidBody>();
					movingHead->lookAt(target->getPosition());
				}

				//---------
				void MovingHeadToWorld::setPanTiltOrAlert(const glm::vec2 & panTilt) {
					try {
						this->throwIfMissingAConnection<DMX::MovingHead>();
						auto movingHead = this->getInput<DMX::MovingHead>();
						movingHead->setPanTilt(panTilt);
					}
					RULR_CATCH_ALL_TO_ALERT;
				}
			}
		}
	}
}