#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			//----------
			Scan::Scan() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Scan::getTypeName() const {
				return "LSS::Scan";
			}

			//----------
			void Scan::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<World>();
				this->addInput<Procedure::Scan::Graycode>();

				this->manageParameters(this->parameters);
			}

			//----------
			void Scan::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				
				{
					inspector->addButton("Scan", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Scan");
							this->scan();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
				}
				{
					inspector->addButton("Triangulate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Triangulate");
							this->triangulate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					})->setHeight(100.0f);
				}
				{
					auto lastCameraTransformButton = inspector->addButton("Clear last transform", [this]() {
						this->lastCameraTransform = glm::mat4(1.0f);
					});
					auto lastCameraTransformButtonWeak = weak_ptr<ofxCvGui::Widgets::Button>(lastCameraTransformButton);
					lastCameraTransformButton->onUpdate += [this, lastCameraTransformButtonWeak](ofxCvGui::UpdateArguments &) {
						auto lastCameraTransformButton = lastCameraTransformButtonWeak.lock();
						lastCameraTransformButton->setCaption(this->lastCameraTransform == glm::mat4(1.0f)
							? "Ready to scan"
							: "Clear last transform");
					};
				}
			}

			//----------
			void Scan::serialize(nlohmann::json & json) {
				
			}

			//----------
			void Scan::deserialize(const nlohmann::json & json) {

			}

			//----------
			void Scan::scan() {
				this->throwIfMissingAnyConnection();
				auto world = this->getInput<World>();
				auto graycode = this->getInput<Procedure::Scan::Graycode>();
				graycode->throwIfMissingAConnection<Item::Camera>();
				auto graycodeVideoOutputPin = graycode->getInputPin<System::VideoOutput>();
				auto priorVideOutputConnection = graycodeVideoOutputPin->getConnection();

				ofColor scanColor;

				try {
					auto camera = graycode->getInput<Item::Camera>();
					auto cameraViewWorld = camera->getViewInWorldSpace();
					//cameraViewWorld.distortion.clear(); //We undistort ourselves
					//auto cameraMatrix = camera->getCameraMatrix();
					//auto distortionCoefficients = camera->getDistortionCoefficients();

					{
						//check if we're scanning the same transform as last time
						auto cameraTransform = camera->getTransform();
						bool sameTransform = cameraTransform == this->lastCameraTransform;
						if (sameTransform) {
							throw(ofxRulr::Exception("Camera extrinsics haven't changes since last scan"));
						}
						else {
							this->lastCameraTransform = cameraTransform;
						}
					}

					auto projectors = world->getProjectors();

					auto scopedProcessScanProjectors = Utils::ScopedProcess("Scan projectors", true, projectors.size());
					for (auto projector : projectors) {
						if (!projector) {
							scopedProcessScanProjectors.nextChildProcess();
							continue;
						}

						auto scopedProcessScanProjector = Utils::ScopedProcess("Scan " + projector->getName(), false);
						{
							projector->throwIfMissingAnyConnection();
							auto videoOutput = projector->getInput<System::VideoOutput>();
							auto itemProjector = projector->getInput<Item::Projector>();

							//pass over closed projectors / muted
							if (!videoOutput->isWindowOpen() || videoOutput->getMute()) {
								continue;
							}

							//set all projectors to black
							for(auto otherProjector : projectors) {
								if (otherProjector == projector) {
									continue;
								}
								auto videoOutput = otherProjector->getInput<System::VideoOutput>();
								if (videoOutput) {
									if (videoOutput->isWindowOpen()) {
										videoOutput->clearFbo(false);
										videoOutput->presentFbo();
									}
								}
							}


							//perform the graycode scan
							if (!this->parameters.useExistingData) {
								graycodeVideoOutputPin->connect(videoOutput);
								graycode->runScan();
							}

							//process the data from the scan
							const auto & dataSet = graycode->getDataSet();
							map<uint32_t, vector<glm::vec2>> cameraPixelsPerProjectorPixel;
							{
								Utils::ScopedProcess scopedProcessGatherData("Gathering data", false);

								//build up camera pixels per projector pixel
								for (const auto & pixel : dataSet) {
									if (pixel.active && pixel.projector != 0) {
										auto cameraPixelXY = pixel.getCameraXY();
										cameraPixelsPerProjectorPixel[pixel.projector].push_back(cameraPixelXY);
									}
								}
							}
							
							auto projectorScan = make_shared<Projector::Scan>();
							{
								Utils::ScopedProcess scopedProcessBuildingCameraRays("Building camera rays", false);

								projectorScan->cameraPosition = camera->getPosition();

								for (const auto & projectorPixelIt : cameraPixelsPerProjectorPixel) {
									//get a camera ray per projector pixel

									auto cameraPixelCount = projectorPixelIt.second.size();

									glm::vec2 centroid;
									if (cameraPixelCount == 0) {
										//avoid issues
										continue;
									}
									else if (cameraPixelCount == 1) {
										centroid = projectorPixelIt.second.front();
									}
									else {
										//otherwise take trimmed mean
										glm::vec2 originalMean;
										{
											for (const auto & cameraPixel : projectorPixelIt.second) {
												originalMean += cameraPixel;
											}
											originalMean /= projectorPixelIt.second.size();
										}

										vector<float> distancePerCameraPixelToMean;
										float thresholdDistance;
										{
											distancePerCameraPixelToMean.reserve(projectorPixelIt.second.size());
											for (const auto & cameraPixel : projectorPixelIt.second) {
												auto distance = glm::distance2(cameraPixel, originalMean);
												distancePerCameraPixelToMean.push_back(distance);
											}

											auto sortedDistances = distancePerCameraPixelToMean;
											std::sort(sortedDistances.begin(), sortedDistances.end());
											//80%
											auto thresholdDistanceIndex = (size_t)((float)sortedDistances.size() * 0.8f);
											thresholdDistance = sortedDistances[thresholdDistanceIndex];
											if (thresholdDistance < 2) {
												thresholdDistance = 2; // minimum threshold is 2px (always include in trimmed mean)
											}
										}

										size_t trimmedMeanCount = 0;
										glm::vec2 trimmedMean;
										for (size_t iCameraPixel = 0; iCameraPixel < projectorPixelIt.second.size(); iCameraPixel++) {
											const auto & distance = distancePerCameraPixelToMean[iCameraPixel];
											if (distance < thresholdDistance) {
												trimmedMean += projectorPixelIt.second[iCameraPixel];
												trimmedMeanCount++;
											}
										}
										trimmedMean /= trimmedMeanCount;

										centroid = trimmedMean;
									}

									//now we have centroid, let's store the ray
									{
										LSS::Projector::ProjectorPixelFind projectorPixel;
										projectorPixel.cameraPixelRay = cameraViewWorld.castPixel(centroid);
										projectorPixel.cameraPixelXY = centroid;
										projectorScan->projectorPixels.emplace(projectorPixelIt.first, projectorPixel);
									}
								}
							}

							{
								//set the color to match the first scan
								if (scanColor == ofColor()) {
									scanColor = projectorScan->color;
								}
								else {
									projectorScan->color = scanColor;
								}
							}
							itemProjector->setWidth(videoOutput->getWidth());
							itemProjector->setHeight(videoOutput->getHeight());
							projector->addScan(projectorScan);

						}
						scopedProcessScanProjector.end();
					}
					scopedProcessScanProjectors.end();

					graycode->getInputPin<System::VideoOutput>()->connect(priorVideOutputConnection);
				}
				catch (...) {
					if (graycode) {
						graycode->getInputPin<System::VideoOutput>()->connect(priorVideOutputConnection);
					}
					throw;
				}
			}

			//----------
			void Scan::triangulate() {
				this->throwIfMissingAConnection<World>();
				auto world = this->getInput<World>();
				auto projectors = world->getProjectors();

				Utils::ScopedProcess triangulateProjectors("Triangulating projectors", false, projectors.size());
				for (auto projector : projectors) {
					projector->triangulate(this->parameters.triangulate.maxResidual);
				}
			}
		}
	}
}