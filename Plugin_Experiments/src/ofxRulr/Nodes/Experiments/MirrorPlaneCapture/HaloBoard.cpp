#include "pch_Plugin_Experiments.h"

#define INCHES_PER_METER (1.0f / 0.0254f)

#define ARUCO_MIP_16h3_NonMirroring {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 78, 79, 80, 81 }

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				HaloBoard::HaloBoard() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				std::string HaloBoard::getTypeName() const {
					return "ArUco::HaloBoard";
				}

				//----------
				void HaloBoard::init() {
					RULR_NODE_UPDATE_LISTENER;

					auto panel = ofxCvGui::Panels::makeImage(this->preview);
					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						//paper sizes
						ofPushMatrix();
						{
							float mmToPixels = 1 / 1000.0f * INCHES_PER_METER * this->parameters.preview.DPI;
							ofScale(mmToPixels, mmToPixels);

							ofColor baseColor(200, 100, 100);
							int index = 0;
							for (auto paperSize : this->paperSizes) {
								auto paperColor = baseColor;
								paperColor.setHue(index * 40);

								ofPushStyle();
								{
									ofSetColor(paperColor);

									//outline
									ofNoFill();
									ofDrawRectangle(0, 0, 297, 210);

									//text
									ofDrawBitmapString(paperSize->name, paperSize->width, paperSize->height);
								}
								ofPopStyle();

								index++;
							}
						}
						ofPopMatrix();
					};
					this->panel = panel;

					this->manageParameters(this->parameters);

					{
						this->paperSizes.emplace(new PaperSize{ "A0", 1189, 841 });
						this->paperSizes.emplace(new PaperSize{ "A1", 841, 594 });
						this->paperSizes.emplace(new PaperSize{ "A2", 594, 420 });
						this->paperSizes.emplace(new PaperSize{ "A3", 420, 297 });
						this->paperSizes.emplace(new PaperSize{ "A4", 297, 210 });
					}

					this->dictionary = aruco::Dictionary::load("ARUCO_MIP_16h3");

					this->detector = make_shared<aruco::MarkerDetector>(this->dictionary.getName());
					{
						auto & params = this->detector->getParameters();
						params.enclosedMarker = true;
					}
				}

				//----------
				void HaloBoard::update() {
					//delete preview if it's wrong
					if (this->parameters.size.width != this->cachedParameters.size.width
						|| this->parameters.size.height != this->cachedParameters.size.height

						|| this->parameters.length.square != this->cachedParameters.length.square
						|| this->parameters.length.outerCorner != this->cachedParameters.length.outerCorner
						
						|| this->parameters.preview.DPI != this->cachedParameters.preview.DPI) {
						//we need to rebuild the preview
						this->board.reset();
					}

					//rebuild the board if needed
					if (!this->board) {
						try {
							auto board = make_shared<Board>();

							// Function for adding corners
							auto addCorner = [board](const Board::Position & position, const Board::MarkerCorner & markerCorner) {
								//find if corner already exists
								Board::BoardCorner * cornerData = nullptr;

								for (auto & corner : board->corners) {
									if (corner.position == position) {
										cornerData = &corner;
									}
								}

								//make one if not
								if (!cornerData) {
									board->corners.push_back(Board::BoardCorner());
									cornerData = &board->corners.back();
									cornerData->position = position;
								}

								cornerData->markerCorners.push_back(markerCorner);
							};

							// Allocate the fbo
							ofFbo previewFbo;
							{
								auto pixelSize = this->getPhysicalSize() * this->getPreviewPixelsPerMeter();
								ofFbo::Settings fboSettings;
								{
									fboSettings.width = pixelSize.x;
									fboSettings.height = pixelSize.y;
									fboSettings.internalformat = GL_RGBA;
								};

								previewFbo.allocate(fboSettings);
							}

							previewFbo.begin();
							{
								ofClear(255, 255);

								ofPushMatrix();
								{
									//change into meters
									auto pixelsPerMeter = this->getPreviewPixelsPerMeter();
									ofScale(pixelsPerMeter, pixelsPerMeter);

									//move in by the outer corner amount
									ofTranslate(this->parameters.length.outerCorner, this->parameters.length.outerCorner);

									//scale by square
									const auto square = this->parameters.length.square;
									ofScale(square, square);

									auto markerIDs = this->getNonMirroringIDs();
									auto markerIDIterator = markerIDs.begin();

									ofImage markerPreview;

									//Iterate over squares
									for (int y = 0; y < this->parameters.size.height; y++) {
										for (int x = 0; x < this->parameters.size.width; x++) {
											if (x % 2 == y % 2) {
												//active squares

												if (markerIDIterator == markerIDs.end()) {
													throw(Exception("Not enough markers available in dictionary for this board design"));
												}

												auto markerID = *markerIDIterator++;

												//make the marker image
												{
													auto markerImage = this->dictionary.getMarkerImage_id(markerID
														, 1
														, false
														, false
														, false);
													cv::Mat markerImageColor;
													cv::cvtColor(markerImage, markerImageColor, CV_GRAY2RGBA);
													ofxCv::copy(markerImageColor, markerPreview);
													markerPreview.update();

													// Turn off filtering
													markerPreview.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
												}

												//render the image into the fbo
												markerPreview.draw(x, y, 1, 1);

												//Store the data
												{
													//Marker
													{
														Board::Marker marker{
														markerID,
														Board::Position {
															x,
															y
														}
														};

														board->markers.emplace(markerID, marker);
													}

													//Corners
													{
														addCorner(Board::Position{ x, y }, Board::MarkerCorner{ markerID, 0 });
														addCorner(Board::Position{ x + 1, y }, Board::MarkerCorner{ markerID, 1 });
														addCorner(Board::Position{ x + 1, y + 1 }, Board::MarkerCorner{ markerID, 2 });
														addCorner(Board::Position{ x, y + 1 }, Board::MarkerCorner{ markerID, 3 });
													}
												}
											}
										}
									}

									//Draw the outside parts
									{
										auto outsideSize = this->parameters.length.outerCorner / this->parameters.length.square;

										ofPushStyle();
										{
											ofFill();
											ofSetLineWidth(0);
											ofSetColor(0, 255);

											auto width = this->parameters.size.width.get();
											auto height = this->parameters.size.height.get();

											//top left
											{
												ofPath arc;
												arc.setCircleResolution(100);
												arc.arc(ofVec2f(0, 0), outsideSize, outsideSize, 180, 270);
												arc.setFillColor(ofColor(0));
												arc.setFilled(true);
												arc.close();
												arc.draw();
											}

											//bottom left
											if (height % 2 == 1) {
												ofPath arc;
												arc.setCircleResolution(100);
												arc.arc(ofVec2f(0, height), outsideSize, outsideSize, 90, 180);
												arc.setFillColor(ofColor(0));
												arc.setFilled(true);
												arc.close();
												arc.draw();
											}

											//top right
											if (width % 2 == 1) {
												ofPath arc;
												arc.setCircleResolution(100);
												arc.arc(ofVec2f(width, 0), outsideSize, outsideSize, 270, 360);
												arc.setFillColor(ofColor(0));
												arc.setFilled(true);
												arc.close();
												arc.draw();
											}

											//bottom right
											if (width % 2 == 1 && height % 2 == 1) {
												ofPath arc;
												arc.setCircleResolution(100);
												arc.arc(ofVec2f(width, height), outsideSize, outsideSize, 0, 90);
												arc.setFillColor(ofColor(0));
												arc.setFilled(true);
												arc.close();
												arc.draw();
											}

											//top edge
											for (int x = 0; x < width; x++) {
												if (x % 2 == 1) {
													ofDrawRectangle(x, -outsideSize, 1, outsideSize);
												}
											}

											//left edge
											for (int y = 0; y < height; y++) {
												if (y % 2 == 1) {
													ofDrawRectangle(-outsideSize, y, outsideSize, 1);
												}
											}

											//bottom edge
											for (int x = 0; x < width; x++) {
												if (x % 2 == height % 2) {
													ofDrawRectangle(x, height, 1, outsideSize);
												}
											}

											//right edge
											for (int y = 0; y < height; y++) {
												if (y % 2 == width % 2) {
													ofDrawRectangle(width, y, outsideSize, 1);
												}
											}
										}
										ofPopStyle();
									}
								}
								ofPopMatrix();
							}
							previewFbo.end();

							//copy the preview into the image
							{
								previewFbo.readToPixels(this->preview.getPixels());
								this->preview.update();
							}

							//sort the board
							{
								sort(board->corners.begin(), board->corners.end(),
									[](const Board::BoardCorner & a, const Board::BoardCorner & b) -> bool
								{
									if (a.position.y == b.position.y) {
										return a.position.x < b.position.x;
									}
									else {
										return a.position.y < b.position.y;
									}
									
								});
							}

							//save the board
							this->board = board;

							//cache parameters
							{
								this->cachedParameters.size.width = this->parameters.size.width;
								this->cachedParameters.size.height = this->parameters.size.height;

								this->cachedParameters.length.square = this->parameters.length.square;
								this->cachedParameters.length.outerCorner = this->parameters.length.outerCorner;

								this->cachedParameters.preview.DPI = this->parameters.preview.DPI;
							}
						}
						RULR_CATCH_ALL_TO_ERROR
					}
				}

				//----------
				void HaloBoard::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
				}

				//----------
				void HaloBoard::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);
				}

				//----------
				void HaloBoard::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addParameterGroup(this->parameters);
					inspector->addLiveValue<ofVec2f>("Physical size [m]", [this]() {
						return this->getPhysicalSize();
					});
				}

				//----------
				//https://github.com/opencv/opencv_contrib/blob/master/modules/aruco/samples/detect_board_charuco.cpp
				bool HaloBoard::findBoard(cv::Mat image, vector<cv::Point2f> & imagePoints, vector<cv::Point3f> & objectPoints, FindBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const {
					if (!this->board) {
						throw(Exception("Board is not setup"));
					}

					cv::Mat imageGrey;
					if (image.channels() == 1) {
						imageGrey = image;
					}
					else {
						cv::cvtColor(image, imageGrey, CV_RGB2GRAY);
					}

					if (this->parameters.detection.openCorners > 0) {
						//get the corners in the image
						vector<cv::Point2f> corners;
						cv::goodFeaturesToTrack(imageGrey
							, corners
							, 2000
							, 0.01
							, 10);

						//draw circles over the corners
						for (auto & corner : corners) {
							cv::circle(imageGrey
								, corner
								, this->parameters.detection.openCorners
								, cv::Scalar(255)
								, -1);
						}
					}

					imagePoints.clear();
					objectPoints.clear();

					//Find the markers
					vector<aruco::Marker> foundMarkers;
					{
						this->detector->detect(imageGrey
							, foundMarkers
							, cameraMatrix
							, distortionCoefficients
							, this->parameters.length.square);

						if (foundMarkers.empty()) {
							return false;
						}
					}

					//Corner finds
					map<int, vector<cv::Point2f>> markerCornerFinds;
					vector<float> markerCornerScale; // scale in pixels for a square meeting at this corner (for subpix)

					for (const auto & marker : foundMarkers) {

						//for each corner of a found marker
						for (int i = 0; i < 4; i++) {
							Board::MarkerCorner markerCorner{
								marker.id,
								i
							};

							//for each corner on the board
							for (int boardCornerIndex = 0; boardCornerIndex < this->board->corners.size(); boardCornerIndex++) {
								const auto & boardCorner = this->board->corners[boardCornerIndex];

								//check if that corner matches this MarkerCorner
								for (const auto & boardMarkerCorner : boardCorner.markerCorners) {
									if (boardMarkerCorner == markerCorner) {
										//store it into there
										markerCornerFinds[boardCornerIndex].push_back(marker[i]);

										//get the length of a square side meeting this corner
										markerCornerScale.push_back((ofxCv::toOf(marker[(i + 1) % 4]) - ofxCv::toOf(marker[i])).length());

										//save some time, nothing else should match in this loop
										break;
									}
								}
							}
						}
					}

					{
						const auto square = this->parameters.length.square.get();
						for (const auto & it : markerCornerFinds) {
							const auto & boardCorner = this->board->corners[it.first];

							//take the mean
							ofVec2f mean;
							{
								ofVec2f accumulate;
								for (const auto & find : it.second) {
									accumulate += ofxCv::toOf(find);
								}
								mean = accumulate / it.second.size();
							}

							const auto & position = this->board->corners[it.first].position;
							
							imagePoints.push_back(ofxCv::toCv(mean));
							objectPoints.push_back(ofxCv::toCv(ofVec3f(position.x, position.y, 0.0f) * square));
						}
					}

					if (imagePoints.empty()) {
						return false;
					}

					//perform sub-pixel refinement
					if(this->parameters.refinement.enabled) {
						float mean = 0.0f;
						{
							float accumulate = 0.0f;
							for (const auto & find : markerCornerScale) {
								accumulate += find;
							}
							mean = accumulate / markerCornerScale.size();
						}

						int subPixSearch = mean / 8.0f; //markers are 6x6 pixels (4x4 data)

						subPixSearch = (subPixSearch / 2) * 2 + 1; // ensure odd
						if (subPixSearch < 1) {
							subPixSearch = 1; // ensure non zero
						}

						int zeroZone = mean * 0.01f;

						cv::cornerSubPix(imageGrey
							, imagePoints
							, cv::Size(subPixSearch
								, subPixSearch)
							, cv::Size(zeroZone
								, zeroZone)
							, cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 50, 1e-6));
					}

					return true;
				}

				//----------
				void HaloBoard::drawObject() const {
					if (this->board) {
						auto pixelsPerMeter = this->getPreviewPixelsPerMeter();
						auto square = this->parameters.length.square;
						auto outerCorner = this->parameters.length.outerCorner;

						this->preview.draw(-outerCorner
							, -outerCorner
							, square * this->parameters.size.width + 2 * outerCorner
							, square * this->parameters.size.height + 2 * outerCorner);
					}
				}

				//----------
				float HaloBoard::getSpacing() const {
					return this->parameters.length.square.get();
				}

				//----------
				ofVec3f HaloBoard::getCenter() const {
					return ofVec3f(this->parameters.size.width * this->parameters.length.square
						, this->parameters.size.height * this->parameters.length.square
						, 0.0f) / 2.0f;
				}

				//----------
				ofxCvGui::PanelPtr HaloBoard::getPanel() {
					return this->panel;
				}

				//----------
				ofVec2f HaloBoard::getPhysicalSize() const {
					return ofVec2f(this->parameters.size.width * this->parameters.length.square
						, this->parameters.size.height * this->parameters.length.square)
						+ (this->parameters.length.outerCorner * 2);
				}

				//----------
				std::vector<int> HaloBoard::getNonMirroringIDs() const {
					return vector<int> ARUCO_MIP_16h3_NonMirroring;
				}

				//----------
				float HaloBoard::getPreviewPixelsPerMeter() const {
					return INCHES_PER_METER * this->parameters.preview.DPI;
				}
			}
		}
	}
}