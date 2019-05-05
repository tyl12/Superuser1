
#clone source code:
#git clone https://github.com/tyl12/Superuser_phhusson.git
#cd Superuser_phhusson/
#git clone https://github.com/tyl12/Widgets.git

#build apk with gradle.
(cd Superuser && ./gradlew assembleDebug)

#build su binary with ndk-build
(cd Superuser/jni && ndk-build FORCE_LOCAL_NDK_BUILD=true)

