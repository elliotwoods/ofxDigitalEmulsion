#include "pch_RulrCore.h"
#include "NodeHost.h"

using namespace ofxAssets;

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			//----------
			NodeHost::NodeHost(shared_ptr<Nodes::Base> node) {
				node->setNodeHost(this);

				/*
				The NodeHost Gui element is a tree of elements:

				NodeHost
					- elements
						- title
						- resizeHandle
						- outputPinViewv
						- nodeView (if any)
					- inputPins
				*/				
				this->elements = make_shared<ofxCvGui::ElementGroup>();
				this->elements->setScissorEnabled(true);

				ofxCvGui::ElementPtr title;

				this->node = node;
				this->nodeView = node->getPanel();
				//check if this node has a view
				if (nodeView) {
					this->elements->add(nodeView);
				}
				
				//we setup a title and add it
				title = make_shared<Element>();
				title->onDraw += [this](ofxCvGui::DrawArguments & args) {
					//draw line between inputs and outs
					ofPushStyle();
					{
						ofSetLineWidth(1.0f);
						ofSetColor(50);
						ofDrawLine(0, 0, 0, args.localBounds.height);
						ofDrawLine(args.localBounds.width - 1, 0, args.localBounds.width - 1, args.localBounds.height); //-1 because of scissor
					}
					ofPopStyle();

					//draw text
					ofPushMatrix();
					{
						ofTranslate(0, args.localBounds.height);
						ofRotateDeg(-90);
						ofxCvGui::Utils::drawText(this->getNodeInstance()->getName(), ofRectangle(0, 0, args.localBounds.height, args.localBounds.width), false);
					}
					ofPopMatrix();
				};
				this->elements->add(title);

				auto resizeHandle = make_shared<ofxCvGui::Utils::Button>();
				resizeHandle->onDrawUp += [](ofxCvGui::DrawArguments & args) {
					image("ofxRulr::resizeHandle").draw(args.localBounds);
				};
				resizeHandle->onDrawDown += [](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(0);
					image("ofxRulr::resizeHandle").draw(args.localBounds);
					ofPopStyle();
				};
				weak_ptr<ofxCvGui::Utils::Button> resizeHandleWeak = resizeHandle;
				resizeHandle->onMouse += [this, resizeHandleWeak](ofxCvGui::MouseArguments & args) {
					auto resizeHandle = resizeHandleWeak.lock();
					if (resizeHandle) {
						if (args.isDragging(resizeHandle.get())) {
							auto newBounds = this->getBounds();
							newBounds.width += args.movement.x;
							newBounds.height += args.movement.y;
							this->setBounds(newBounds);
						}
					}
				};
				this->elements->add(resizeHandle);

				auto outputPinView = make_shared<PinView>();
				outputPinView->setup(*this->getNodeInstance());
				this->elements->add(outputPinView);

				this->elements->onBoundsChange += [resizeHandle, title, outputPinView, this](ofxCvGui::BoundsChangeArguments & args) {
					this->inputPins->setBounds(ofRectangle(0, 0, RULR_NODEHOST_INPUTAREA_WIDTH, this->getHeight()));
					
					auto titleX = RULR_NODEHOST_INPUTAREA_WIDTH;

					if (this->nodeView) {
						auto viewBounds = args.localBounds;
						viewBounds.x = RULR_NODEHOST_INPUTAREA_WIDTH;
						viewBounds.width -= RULR_NODEHOST_INPUTAREA_WIDTH + RULR_NODEHOST_OUTPUTAREA_WIDTH + RULR_NODEHOST_TITLE_WIDTH;
						viewBounds.y = 1;
						viewBounds.height -= 2;
						this->nodeView->setBounds(viewBounds);

						titleX = viewBounds.getRight();
					}

					title->setBounds(ofRectangle(titleX, 0, RULR_NODEHOST_TITLE_WIDTH, args.localBounds.height));

					this->outputPinPosition = ofVec2f(this->getWidth(), this->getHeight() / 2.0f);
					const auto iconSize = 48;
					outputPinView->setBounds(ofRectangle(this->getOutputPinPosition() - ofVec2f(iconSize + 20, iconSize / 2), iconSize, iconSize));

					auto & resizeImage = image("ofxRulr::resizeHandle");
					resizeHandle->setBounds(ofRectangle(args.localBounds.width - resizeImage.getWidth(), args.localBounds.height - resizeImage.getHeight(), resizeImage.getWidth(), resizeImage.getHeight()));

				};

				this->inputPins = make_shared<ofxCvGui::ElementGroup>();
				for (auto inputPin : node->getInputPins()) {
					this->inputPins->add(inputPin);
					weak_ptr<AbstractPin> inputPinWeak = inputPin;
					inputPin->onBeginMakeConnection += [this, inputPinWeak](ofEventArgs &) {
						auto inputPin = inputPinWeak.lock();
						if (inputPin) {
							this->onBeginMakeConnection(inputPin);
						}
					};
					inputPin->onReleaseMakeConnection += [this, inputPinWeak](ofxCvGui::MouseArguments & args) {
						auto inputPin = inputPinWeak.lock();
						if (inputPin) {
							this->onReleaseMakeConnection(args);
						}
					};
					inputPin->onDeleteConnectionUntyped += [this, inputPinWeak](shared_ptr<Nodes::Base> &) {
						auto inputPin = inputPinWeak.lock();
						if (inputPin) {
							this->onDropInputConnection(inputPin);
						}
					};
				};
				this->inputPins->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					this->inputPins->layoutGridVertical();
				};
				this->inputPins->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetLineWidth(1.0f);
					ofSetColor(50);
					bool first = true;
					for (auto pin : this->inputPins->getElements()) {
						if (first) {
							first = false;
						}
						else {
							ofDrawLine(pin->getBounds().getTopLeft(), pin->getBounds().getTopRight());
						}
					}
					ofPopStyle();
				}, this, -1);

				this->onUpdate += [this](ofxCvGui::UpdateArguments & args) {
					//--
					//Clamp bounds
					//--
					//
					const int minHeight = MAX(150, (int) this->getNodeInstance()->getInputPins().size() * 75);
					auto bounds = this->getBounds();
					if (this->nodeView) {
						//this node has a view
						const int minWidth = RULR_NODEHOST_INPUTAREA_WIDTH + RULR_NODEHOST_OUTPUTAREA_WIDTH + 200;
						if (bounds.width < minWidth || bounds.height < minHeight) {
							auto fixedBounds = bounds;
							fixedBounds.width = MAX(bounds.width, minWidth);
							fixedBounds.height = MAX(bounds.height, minHeight);
							this->setBounds(fixedBounds);
						}
					}
					else {
						//this node has no view
						const auto nodeHostWidth = RULR_NODEHOST_INPUTAREA_WIDTH + RULR_NODEHOST_OUTPUTAREA_WIDTH + RULR_NODEHOST_TITLE_WIDTH;
						if (bounds.width != nodeHostWidth || bounds.height < minHeight) {
							bounds.width = nodeHostWidth;
							bounds.height = MAX(bounds.height, minHeight);
							this->setBounds(bounds);
						}
					}
					//
					//--
				};

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					{
						//shadow for node
						ofFill();
						ofSetColor(0, 100);
						ofPushMatrix();
						{
							ofTranslate(5, 5);
							ofDrawRectangle(this->getLocalBounds());
						}
						ofPopMatrix();

						//background for node
						ofSetColor(80);
						ofDrawRectangle(this->getLocalBounds());

						if (this->nodeView) {
							//background for nodeView
							ofSetColor(30);
							ofDrawRectangle(this->nodeView->getBounds());
						}
					}
					ofPopStyle();


					//output pin
					ofPushStyle();
					{
						ofPushMatrix();
						{
							ofTranslate(this->getOutputPinPosition());
							//
							ofSetColor(this->getNodeInstance()->getColor());
							ofSetLineWidth(0.0f);
							ofDrawRectangle(-10, -3, 10, 6);
							//
						}
						ofPopMatrix();
					}
					ofPopStyle();
				};

				this->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.isDragging(this)) {
						auto newBounds = this->getBounds();
						newBounds.x += args.movement.x;
						newBounds.y += args.movement.y;
						if (newBounds.x < 0) {
							newBounds.x = 0;
						}
						if (newBounds.y < 0) {
							newBounds.y = 0;
						}
						this->setBounds(newBounds);
					}
					
					//if no children took the mouse press, then we'll have it
					args.takeMousePress(this);

					if (args.action == ofxCvGui::MouseArguments::Pressed) {
						// mouse went down somewhere inside this element (regardless of where it's taken)
						ofxCvGui::inspect(this->getNodeInstance());
					}
				};

				this->elements->addListenersToParent(this, true);
				this->inputPins->addListenersToParent(this);

				this->setBounds(ofRectangle(200, 200, 200, 200));
			}

			//----------
			shared_ptr<Nodes::Base> NodeHost::getNodeInstance() {
				return this->node;
			}

			//----------
			ofVec2f NodeHost::getInputPinPosition(shared_ptr<AbstractPin> pin) const {
				for (auto inputPin : this->inputPins->getElements()) {
					if (inputPin == pin) {
						return pin->getPinHeadPosition() + pin->getBounds().getTopLeft() + this->inputPins->getBounds().getTopLeft() + this->getBounds().getTopLeft();
					}
				}

				//throw an error if we didn't find it
				auto pointerValue = (size_t) pin.get();
				throw(ofxRulr::Exception("NodeHost::getInputPinPosition can't find input pin" + ofToString(pointerValue)));
			}

			//----------
			ofVec2f NodeHost::getOutputPinPositionGlobal() const {
				return this->getOutputPinPosition() + this->getBounds().getTopLeft();
			}

			//----------
			ofVec2f NodeHost::getOutputPinPosition() const {
				return this->outputPinPosition;
			}

			//----------
			void NodeHost::serialize(nlohmann::json & json) {
				json["Bounds"] << this->getBounds();

				//seriaise type name and content
				auto node = this->getNodeInstance();
				json["NodeTypeName"] = node->getTypeName();
				json["Name"] = node->getName();
				node->serialize(json["Content"]);
			}
		}
	}
}