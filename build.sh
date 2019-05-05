#build apk with gradle.
(cd Superuser && ./gradlew assembleDebug)

#build su binary with ndk-build
(cd Superuser && ndk-build)

