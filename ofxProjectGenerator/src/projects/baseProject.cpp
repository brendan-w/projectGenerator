//
//  baseProject.cpp
//  projectGenerator
//
//  Created by molmol on 3/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "baseProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "ofConstants.h"
using namespace std;

void baseProject::setup(string _target){
    target = _target;
    templatePath = ofFilePath::join(getOFRoot(),"scripts/" + target + "/template/");
    setup(); // call the inherited class setup(), now that target is set.
}

// todo: this bool param should be removed.
bool baseProject::create(string path,  bool bParseAddonsDotMake){
    
    addons.clear();

    if(!ofFilePath::isAbsolute(path)){
    	path = (std::filesystem::current_path() / std::filesystem::path(path)).string();
    }
    projectDir = ofFilePath::addTrailingSlash(path);
    projectName = ofFilePath::getFileName(path);
    bool bDoesDirExist = false;

    ofDirectory project(ofFilePath::join(projectDir,"src"));    // this is a directory, really?
    if(project.exists()){
        bDoesDirExist = true;
    }else{
        ofDirectory project(projectDir);
        ofDirectory(ofFilePath::join(templatePath,"src")).copyTo(ofFilePath::join(projectDir,"src"));
        ofDirectory(ofFilePath::join(templatePath,"bin")).copyTo(ofFilePath::join(projectDir,"bin"));
    }

    // if overwrite then ask for permission...

    bool ret = createProjectFile();
    if(!ret) return false;

    ret = loadProjectFile();
    if(!ret) return false;

    if (bDoesDirExist){
        vector < string > fileNames;
        getFilesRecursively(ofFilePath::join(projectDir , "src"), fileNames);

        for (int i = 0; i < (int)fileNames.size(); i++){

            fileNames[i].erase(fileNames[i].begin(), fileNames[i].begin() + projectDir.length());

            string first, last;
#ifdef TARGET_WIN32
            splitFromLast(fileNames[i], "\\", first, last);
#else
            splitFromLast(fileNames[i], "/", first, last);
#endif
            if (fileNames[i] != "src/ofApp.cpp" &&
                fileNames[i] != "src/ofApp.h" &&
                fileNames[i] != "src/main.cpp" &&
                fileNames[i] != "src/ofApp.mm" &&
                fileNames[i] != "src/main.mm"){
                addSrc(fileNames[i], first);
            }
        }

//		if( target == "ios" ){
//			getFilesRecursively(ofFilePath::join(projectDir , "bin/data"), fileNames);
//
//	        for (int i = 0; i < (int)fileNames.size(); i++){
//				fileNames[i].erase(fileNames[i].begin(), fileNames[i].begin() + projectDir.length());
//
//				string first, last;
//				splitFromLast(fileNames[i], "/", first, last);
//				if (fileNames[i] != "Default.png" &&
//					fileNames[i] != "src/ofApp.h" &&
//					fileNames[i] != "src/main.cpp" &&
//					fileNames[i] != "src/ofApp.mm" &&
//					fileNames[i] != "src/main.mm"){
//					addSrc(fileNames[i], first);
//				}
//			}
//		}

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    		if (bParseAddonsDotMake) parseAddons();
#endif
        // get a unique list of the paths that are needed for the includes.
        list < string > paths;
        vector < string > includePaths;
        for (int i = 0; i < (int)fileNames.size(); i++){
            size_t found;
    #ifdef TARGET_WIN32
            found = fileNames[i].find_last_of("\\");
    #else
            found = fileNames[i].find_last_of("/");
    #endif
            paths.push_back(fileNames[i].substr(0,found));
        }

        paths.sort();
        paths.unique();
        for (list<string>::iterator it=paths.begin(); it!=paths.end(); ++it){
            includePaths.push_back(*it);
        }

        for (int i = 0; i < includePaths.size(); i++){
            addInclude(includePaths[i]);
        }
    }
    return true;
}

bool baseProject::save(bool createMakeFile){

    // only save an addons.make file if requested on ANY platform
    // this way we don't thrash the git repo for our examples, but
    // we do make the addons.make file for any new projects...that
    // way it can be distributed and re-used by others with the PG

    if(createMakeFile){
        
        ofLog(OF_LOG_NOTICE) << "saving addons.make";
        ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"), ofFile::WriteOnly);
        for(int i = 0; i < addons.size(); i++){
            addonsMake << addons[i].name << endl;
        }
        
        
    }

	return saveProjectFile();
}

void baseProject::addAddon(ofAddon & addon){
    for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name) return;
	}

	addons.push_back(addon);

    for(int i=0;i<(int)addon.includePaths.size();i++){
        ofLogNotice() << "adding addon include path: " << addon.includePaths[i];
        addInclude(addon.includePaths[i]);
    }
    for(int i=0;i<(int)addon.libs.size();i++){
        ofLogNotice() << "adding addon libs: " << addon.libs[i].path;
        addLibrary(addon.libs[i]);
    }
    for(int i=0;i<(int)addon.cflags.size();i++){
        ofLogNotice() << "adding addon cflags: " << addon.cflags[i];
        addCFLAG(addon.cflags[i]);
    }
	for(int i=0;i<(int)addon.cppflags.size();i++){
	    ofLogNotice() << "adding addon cppflags: " << addon.cppflags[i];
        addCPPFLAG(addon.cppflags[i]);
    }
    for(int i=0;i<(int)addon.ldflags.size();i++){
        ofLogNotice() << "adding addon ldflags: " << addon.ldflags[i];
        addLDFLAG(addon.ldflags[i]);
    }
    for(int i=0;i<(int)addon.srcFiles.size(); i++){
        ofLogNotice() << "adding addon srcFiles: " << addon.srcFiles[i];
        addSrc(addon.srcFiles[i],addon.filesToFolders[addon.srcFiles[i]]);
    }
    for(int i=0;i<(int)addon.csrcFiles.size(); i++){
        ofLogNotice() << "adding addon c srcFiles: " << addon.csrcFiles[i];
        addSrc(addon.csrcFiles[i],addon.filesToFolders[addon.csrcFiles[i]],C);
    }
    for(int i=0;i<(int)addon.cppsrcFiles.size(); i++){
        ofLogNotice() << "adding addon cpp srcFiles: " << addon.cppsrcFiles[i];
        addSrc(addon.cppsrcFiles[i],addon.filesToFolders[addon.cppsrcFiles[i]],CPP);
    }
    for(int i=0;i<(int)addon.objcsrcFiles.size(); i++){
        ofLogNotice() << "adding addon objc srcFiles: " << addon.objcsrcFiles[i];
        addSrc(addon.objcsrcFiles[i],addon.filesToFolders[addon.objcsrcFiles[i]],OBJC);
    }
    for(int i=0;i<(int)addon.headersrcFiles.size(); i++){
        ofLogNotice() << "adding addon header srcFiles: " << addon.headersrcFiles[i];
        addSrc(addon.headersrcFiles[i],addon.filesToFolders[addon.headersrcFiles[i]],HEADER);
    }
}

void baseProject::parseAddons(){
	ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"));
	ofBuffer addonsMakeMem;
	addonsMake >> addonsMakeMem;
	while(!addonsMakeMem.isLastLine()){
	    string line = addonsMakeMem.getNextLine();
	    if(line[0] == '#') continue;
        if(ofTrim(line) == "") continue;
		ofAddon addon;
		cout << projectDir << endl;
		addon.pathToOF = getOFRelPath(projectDir);
		cout << addon.pathToOF << endl;
		addon.fromFS(ofFilePath::join(ofFilePath::join(getOFRoot(), "addons"), line),target);
		addAddon(addon);
	}
}
