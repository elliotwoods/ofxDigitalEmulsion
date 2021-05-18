#pragma once

#include "NodeHost.h"
#include "LinkHost.h"
#include "NodeBrowser.h"

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Graph/FactoryRegister.h"
#include "ofxCvGui/Panels/ElementCanvas.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class Patch : public Nodes::Base {
			public:
				typedef map<NodeHost::Index, shared_ptr<NodeHost> > NodeHostSet;
				typedef map<LinkHost::Index, shared_ptr<LinkHost> > LinkHostSet;

				class View : public ofxCvGui::Panels::ElementCanvas {
				public:
					View(Patch &);
					//const shared_ptr<ofxCvGui::Panels::Base> findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) override;
					void markDirty();

					shared_ptr<NodeHost> getNodeHostUnderCursor(const glm::vec2 & cursorInCanvas);
					shared_ptr<NodeHost> getNodeHostUnderCursor();

					const ofxCvGui::PanelPtr findScreen(const glm::vec2 & xy, ofRectangle & currentPanelBounds) override;
				protected:
					void rebuild();

					ofVec2f lastCursorPositionInCanvas;
					void drawGridLines();
					Patch & patchInstance;
					shared_ptr<NodeBrowser> nodeBrowser;
					ofVec2f birthLocation;

					shared_ptr<ofTexture> cell;
					bool dirty = true;
				};
				Patch();
				virtual ~Patch();
				string getTypeName() const override;
				void init();

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void insertPatchlet(const nlohmann::json &, bool useNewIDs, const glm::vec2 & offset = glm::vec2(0, 0));

				ofxCvGui::PanelPtr getPanel() override;
				void update();
				void drawWorldStage();

				void rebuildLinkHosts();
				const NodeHostSet & getNodeHosts() const;
				const LinkHostSet & getLinkHosts() const;

				shared_ptr<NodeHost> addNode(NodeHost::Index index, shared_ptr<Nodes::Base>, const ofRectangle & bounds = ofRectangle());
				shared_ptr<NodeHost> addNode(shared_ptr<Nodes::Base>, const ofRectangle & bounds = ofRectangle());
				shared_ptr<NodeHost> addNewNode(shared_ptr<BaseFactory>, const ofRectangle & bounds = ofRectangle());

				void addNodeHost(shared_ptr<NodeHost>, int index);
				void addNodeHost(shared_ptr<NodeHost>);

				void deleteSelection();
				void cut();
				void copy();
				void paste();

				shared_ptr<TemporaryLinkHost> getNewLink() const;
				shared_ptr<NodeHost> findNodeHost(shared_ptr<Nodes::Base>) const;
				shared_ptr<NodeHost> getNodeHost(NodeHost::Index) const;
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				NodeHost::Index getNextFreeNodeHostIndex() const;
				LinkHost::Index getNextFreeLinkHostIndex() const;
				void callbackBeginMakeConnection(shared_ptr<NodeHost> targetNodeHost, shared_ptr<AbstractPin> targetPin);
				void callbackReleaseMakeConnection(ofxCvGui::MouseArguments &);

				NodeHostSet nodeHosts;
				LinkHostSet linkHosts;
				shared_ptr<View> view;

				shared_ptr<TemporaryLinkHost> newLink;
				weak_ptr<NodeHost> selection;

				struct Parameters : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<bool> nodes{ "Nodes", true };
						ofParameter<bool> links{ "Links", true };
						ofParameter<bool> grid{ "Grid", true };
						PARAM_DECLARE("Draw", enabled, nodes, links, grid);
					} draw;
					
					PARAM_DECLARE("Patch", draw);
				} parameters;

				Parameters cachedParameters;
			};
		}
	}
}