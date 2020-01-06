#!groovy
/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

properties([ 
  disableConcurrentBuilds()   // prevent parallel builds
])

parallel (
  Linux: {
    node('Frank') {    
      ws("workspace/Hlasm_Plugin/${env.JOB_BASE_NAME}") {
    
        // delete workspace
        stage('[L] Clear workspace') {
          sh 'sudo chown -R jenkins:jenkins ./* || true'
          deleteDir()
        }
        
        // Mark the code checkout 'stage'....
        stage('[L] Checkout') {
          
          // Checkout code from repository
          dir('HlasmPlugin') {
            /*checkout([
              $class: 'GitSCM',
              branches: [[name: env.BRANCH_NAME]],
              doGenerateSubmoduleConfigurations: false,
              extensions: [[$class: 'SubmoduleOption',
                disableSubmodules: false,
                parentCredentials: true,
                recursiveSubmodules: true,
                reference: '',
                trackingSubmodules: false]],
              submoduleCfg: [],
              userRemoteConfigs: [[credentialsId: 'e965a9af-098d-43ca-9b7a-17ee1344f223',
                url: 'https://github.gwd.broadcom.net/MFD/HlasmPlugin.git']]
            ])
            */
            checkout scm
          }
          
          dir('machines') {
            git url: 'https://github.gwd.broadcom.net/mb890989/hlasmplugin-build.git',
              branch: 'master',
              credentialsId: 'e965a9af-098d-43ca-9b7a-17ee1344f223'
          }
        }
        
        def version = ''    
        
        stage('[L] Version') {
          sh "cd ./HlasmPlugin/clients/vscode-hlasmplugin && node -e \"console.log(require('./package.json').version)\" > vscode-hlasmpluginVersion"
          version = readFile("./HlasmPlugin/clients/vscode-hlasmplugin/vscode-hlasmpluginVersion").trim()
          echo "vscode-hlasmplugin version: ${version}"
        }
        
        def buildImageAlpine = ''
        def buildImageGnu = ''
        def buildImageClang = ''
        
        stage('[L] Build docker images') {
          buildImageAlpine = docker.build("build-image-alpine", "./machines/alpine")
          buildImageGnu = docker.build("gnu-19.04", "./machines/gnu-19.04")
          buildImageClang = docker.build("clang-19.04", "./machines/clang-19.04")
        }
        
        stage('[L] Compile') {
          parallel (
            Alpine: {
              buildImageAlpine.inside('-u 0:0')  {
                sh 'mkdir -p build/alpine'
                sh 'cd ./build/alpine && cmake -G Ninja ../../HlasmPlugin && cmake --build . -- -j 2 && cd bin && ./library_test && ./server_test'
              }
            },
            Gnu: {
              buildImageGnu.inside('-u 0:0')  {
                sh 'mkdir -p build/gnu'
                sh 'cd ./build/gnu && rm -rf * && cmake -G Ninja ../../HlasmPlugin && cmake --build . -- -j 2 && cd bin && ./library_test && ./server_test'
              }
            },
            Clang: {
              buildImageClang.inside('-u 0:0 --cap-add SYS_PTRACE')  {
                sh 'mkdir -p build/clang'
                sh 'cd ./build/clang && cmake -DCMAKE_CXX_COMPILER=clang++-8 -DCMAKE_C_COMPILER=clang-8 -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined,fuzzer-no-link -fprofile-instr-generate -fcoverage-mapping" ../../HlasmPlugin && cmake --build . -- -j 9 && cd bin && LLVM_PROFILE_FILE="library_profile" ./library_test && LLVM_PROFILE_FILE="server_profile" ./server_test'
                sh 'cd ./build/clang/bin && llvm-profdata-8 merge -o hlasm_profile library_profile server_profile'
                sh 'cd ./build/clang/bin && llvm-cov-8 export -format=text -instr-profile ./hlasm_profile ./library_test ./server_test > coverage.json'
              }
            }         
          )
        }
        
        stage('[L] Archive artifacts') {
          // Add version to file name
          sh "cd ./build/alpine/bin && sudo mv vscode-hlasmplugin.vsix vscode-hlasmplugin-${version}.vsix"
          sh "cd ./build/gnu/bin && sudo mv vscode-hlasmplugin.vsix vscode-hlasmplugin-${version}.vsix"
          sh "cd ./build/clang/bin && sudo mv vscode-hlasmplugin.vsix vscode-hlasmplugin-${version}.vsix"
          
          archiveArtifacts artifacts: 'build/*/bin/*.vsix,build/clang/bin/coverage.*'
        }
        
        stage ('[L] Upload to Artifactory') {
          // Create 'latest' artifact
          sh 'sudo cp ./build/alpine/bin/vscode-hlasmplugin*.vsix ./build/alpine/bin/vscode-hlasmplugin-latest.vsix'
          sh 'sudo cp ./build/gnu/bin/vscode-hlasmplugin*.vsix ./build/gnu/bin/vscode-hlasmplugin-latest.vsix'
          sh 'sudo cp ./build/clang/bin/vscode-hlasmplugin*.vsix ./build/clang/bin/vscode-hlasmplugin-latest.vsix'
          // Obtain an Artifactory server instance, defined in Jenkins --> Manage:
          server = Artifactory.server 'Test_Artifactory'
          // Configure upload of artifact
          def uploadSpec = """{
            "files": [
              {
                "pattern": "./build/alpine/bin/vscode-hlasmplugin*.vsix",
                "target": "local-files/hlasm/alpine/"
              },
              {
                "pattern": "./build/gnu/bin/vscode-hlasmplugin*.vsix",
                "target": "local-files/hlasm/gnu/"
              },
              {
                "pattern": "./build/clang/bin/vscode-hlasmplugin*.vsix",
                "target": "local-files/hlasm/clang/"
              }
            ]
          }"""
          server.upload spec: uploadSpec
        }
      }
    } 
  },
  Windows: {
    node('Kenny') {
      // set workspace directories
      def BLDX86 = env.CHANGE_ID + 'b' + env.BUILD_NUMBER + 'x86'
      def BLDX64 = env.CHANGE_ID + 'b' + env.BUILD_NUMBER + 'x64'
      
      // delete workspace
      stage('[W] Clear workspace') {
        deleteDir()
        bat "if exist \\${BLDX86} rd /q/s \\${BLDX86}"
        bat "if exist \\${BLDX64} rd /q/s \\${BLDX64}"
      }
    
      // Mark the code checkout 'stage'....
      stage('[W] Checkout') {
        
        // Checkout code from repository
        dir('HlasmPlugin') {
          /*checkout([
            $class: 'GitSCM',
            branches: [[name: env.BRANCH_NAME]],
            doGenerateSubmoduleConfigurations: false,
            extensions: [[$class: 'SubmoduleOption',
              disableSubmodules: false,
              parentCredentials: true,
              recursiveSubmodules: true,
              reference: '',
              trackingSubmodules: false]],
            submoduleCfg: [],
            userRemoteConfigs: [[credentialsId: 'e965a9af-098d-43ca-9b7a-17ee1344f223',
              url: 'https://github.gwd.broadcom.net/MFD/HlasmPlugin.git']]
          ])*/
          checkout scm
        }
      }
      
      def version = ''
      
      stage('[W] Version') {
        version = bat returnStdout: true, script: 'cd .\\HlasmPlugin\\clients\\vscode-hlasmplugin && node -e "console.log(require(\'./package.json\').version)" > vscode-hlasmpluginVersion'
        version = readFile(".\\HlasmPlugin\\clients\\vscode-hlasmplugin\\vscode-hlasmpluginVersion").trim()
        echo "vscode-hlasmplugin version: ${version}"
      }
      
      stage('[W] Compile') {
        parallel (
          x86: {
            bat """
              mkdir \\${BLDX86} && cd \\${BLDX86}
              pushd \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Auxiliary\\Build\" && vcvars32.bat && popd && cmake -D CMAKE_BUILD_TYPE=Release -A win32 ${env.WORKSPACE}\\HlasmPlugin && cmake --build .\\ --config Release && cd bin && library_test.exe && server_test.exe
            """
          },
          x64: {
            bat """
              mkdir \\${BLDX64} && cd \\${BLDX64}
              pushd \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Auxiliary\\Build\" && vcvars64.bat && popd && cmake -D CMAKE_BUILD_TYPE=Release -A x64 ${env.WORKSPACE}\\HlasmPlugin && cmake --build .\\ --config Release && cd bin && library_test.exe && server_test.exe
            """
          }
        )
      }
      
      stage('[w] Archive artifacts') {
      // Add version to file name
        bat "move /Y \\${BLDX86} ${env.WORKSPACE}\\HlasmPlugin\\buildx86"
        bat "cd ${env.WORKSPACE}\\HlasmPlugin\\buildx86\\bin && rename vscode-hlasmplugin.vsix vscode-hlasmplugin-${version}.vsix"
        bat "move /Y \\${BLDX64} ${env.WORKSPACE}\\HlasmPlugin\\buildx64"
        bat "cd ${env.WORKSPACE}\\HlasmPlugin\\buildx64\\bin && rename vscode-hlasmplugin.vsix vscode-hlasmplugin-${version}.vsix"
        
        archiveArtifacts artifacts: 'HlasmPlugin\\build*\\bin\\*.exe,HlasmPlugin\\build*\\bin\\*.dll,HlasmPlugin\\build*\\bin\\*.lib,HlasmPlugin\\build*\\bin\\*.vsix,HlasmPlugin\\build*\\bin\\test\\**', excludes: 'x64\\Release\\*.*pdb,x64\\Release\\*.*obj'
      }
      
      stage ('[L] Upload to Artifactory') {
        // Create 'latest' artifact
        bat "copy /Y/B ${env.WORKSPACE}\\HlasmPlugin\\buildx86\\bin\\vscode-hlasmplugin*.vsix ${env.WORKSPACE}\\HlasmPlugin\\buildx86\\bin\\vscode-hlasmplugin-latest.vsix"
        bat "copy /Y/B ${env.WORKSPACE}\\HlasmPlugin\\buildx64\\bin\\vscode-hlasmplugin*.vsix ${env.WORKSPACE}\\HlasmPlugin\\buildx64\\bin\\vscode-hlasmplugin-latest.vsix"
        // Obtain an Artifactory server instance, defined in Jenkins --> Manage:
        server = Artifactory.server 'Test_Artifactory'
        // Configure upload of artifact
        def uploadSpec = """{
          "files": [
            {
              "pattern": "./HlasmPlugin/buildx86/bin/vscode-hlasmplugin*.vsix",
              "target": "local-files/hlasm/win_x86/"
            },
            {
              "pattern": "./HlasmPlugin/buildx64/bin/vscode-hlasmplugin*.vsix",
              "target": "local-files/hlasm/win_x64/"
            }
          ]
        }"""
        server.upload spec: uploadSpec
      }
    }
  }
)