package org.lflang.generator.uc

import org.apache.commons.text.StringEscapeUtils
import org.lflang.generator.CodeMap
import org.lflang.generator.LFGeneratorContext
import org.lflang.target.property.BuildTypeProperty
import org.lflang.target.property.type.BuildTypeType.BuildType
import org.lflang.target.property.type.PlatformType
import org.lflang.toUnixString
import org.lflang.util.FileUtil
import org.lflang.util.LFCommand
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

class UcStandaloneGenerator(generator: UcGenerator) :
    UcPlatformGenerator(generator) {

    companion object {
        fun buildTypeToCmakeConfig(type: BuildType) = when (type) {
            BuildType.TEST -> "Debug"
            else           -> type.toString()
        }

        const val DEFAULT_BASE_IMAGE: String = "alpine:latest"
    }

    override fun generatePlatformFiles() {
        val srcGenRoot = fileConfig.srcGenBasePath

        // generate the main source file (containing main())
        val mainGenerator = UcMainGenerator(mainReactor, generator.targetConfig, generator.fileConfig)

        val mainSourceFile = Paths.get("lf_main.c")
        val mainHeaderFile = Paths.get("lf_main.h")

        val mainCodeMap = CodeMap.fromGeneratedCode(mainGenerator.generateMainSource())

        cppSources.add(mainSourceFile)
        codeMaps[fileConfig.srcGenPath.resolve(mainSourceFile)] = mainCodeMap

        println("Path: $srcGenPath $srcGenPath")

        FileUtil.writeToFile(mainCodeMap.generatedCode, srcGenPath.resolve(mainSourceFile), true)
        FileUtil.writeToFile(mainGenerator.generateMainHeader(), srcGenPath.resolve(mainHeaderFile), true)

        // generate the cmake scripts
        val cmakeGenerator = UcCmakeGenerator(targetConfig, generator.fileConfig)
        val makeGenerator = UcMakeGenerator(targetConfig, generator.fileConfig)
        val pkgName = fileConfig.srcGenPkgPath.fileName.toString()
//        FileUtil.writeToFile(cmakeGenerator.generateRootCmake(pkgName), srcGenRoot.resolve("CMakeLists.txt"), true)
        FileUtil.writeToFile(cmakeGenerator.generateCmake(cppSources), srcGenPath.resolve("CMakeLists.txt"), true)
        FileUtil.writeToFile(makeGenerator.generateMake(cppSources), srcGenPath.resolve("Makefile"), true)
//        FileUtil.writeToFile("", srcGenPath.resolve(".lf-cpp-marker"), true)
//        var subdir = srcGenPath.parent
//        while (subdir != srcGenRoot) {
//            FileUtil.writeToFile(cmakeGenerator.generateSubdirCmake(), subdir.resolve("CMakeLists.txt"), true)
//            FileUtil.writeToFile("", subdir.resolve(".lf-cpp-marker"), true)
//            subdir = subdir.parent
//        }
    }

    override fun doCompile(context: LFGeneratorContext, onlyGenerateBuildFiles: Boolean): Boolean {

        // make sure the build directory exists
        Files.createDirectories(fileConfig.buildPath)

        val version = checkCmakeVersion()
        var parallelize = true
        if (version != null && version.compareVersion("3.12.0") < 0) {
            messageReporter.nowhere().warning("CMAKE is older than version 3.12. Parallel building is not supported.")
            parallelize = false
        }

        if (version != null) {
            val cmakeReturnCode = runCmake(context)

            if (cmakeReturnCode == 0 && !onlyGenerateBuildFiles) {
                // If cmake succeeded, run make
                val makeCommand = createMakeCommand(fileConfig.buildPath, parallelize, fileConfig.name)
                val makeReturnCode = UcValidator(fileConfig, messageReporter, codeMaps).run(makeCommand, context.cancelIndicator)
                var installReturnCode = 0
                if (makeReturnCode == 0) {
                    val installCommand = createMakeCommand(fileConfig.buildPath, parallelize, "install")
                    installReturnCode = installCommand.run(context.cancelIndicator)
                    if (installReturnCode == 0) {
                        println("SUCCESS (compiling generated C code)")
                        println("Generated source code is in ${fileConfig.srcGenPath}")
                        println("Compiled binary is in ${fileConfig.binPath}")
                    }
                }
                if ((makeReturnCode != 0 || installReturnCode != 0) && !messageReporter.errorsOccurred) {
                    // If errors occurred but none were reported, then the following message is the best we can do.
                    messageReporter.nowhere().error("make failed with error code $makeReturnCode")
                }
            }
            if (cmakeReturnCode != 0) {
                messageReporter.nowhere().error("cmake failed with error code $cmakeReturnCode")
            }
        }
        return !messageReporter.errorsOccurred
    }

    private fun checkCmakeVersion(): String? {
        // get the installed cmake version and make sure it is at least 3.5
        val cmd = commandFactory.createCommand("cmake", listOf("--version"), fileConfig.buildPath)
        var version: String? = null
        if (cmd != null && cmd.run() == 0) {
            val regex = "\\d+(\\.\\d+)+".toRegex()
            version = regex.find(cmd.output.toString())?.value
        }
        if (version == null || version.compareVersion("3.5.0") < 0) {
            messageReporter.nowhere(
            ).error(
                "The C++ target requires CMAKE >= 3.5.0 to compile the generated code. " +
                        "Auto-compiling can be disabled using the \"no-compile: true\" target property."
            )
            return null
        }

        return version
    }


    /**
     * Run CMake to generate build files.
     * @return True, if cmake run successfully
     */
    private fun runCmake(context: LFGeneratorContext): Int {
        val cmakeCommand = createCmakeCommand(fileConfig.buildPath, fileConfig.outPath)
        return cmakeCommand.run(context.cancelIndicator)
    }

    private fun String.compareVersion(other: String): Int {
        val a = this.split(".").map { it.toInt() }
        val b = other.split(".").map { it.toInt() }
        for (x in (a zip b)) {
            val res = x.first.compareTo(x.second)
            if (res != 0)
                return res
        }
        return 0
    }

    private fun getMakeArgs(buildPath: Path, parallelize: Boolean, target: String): List<String> {
        val cmakeConfig = buildTypeToCmakeConfig(targetConfig.get(BuildTypeProperty.INSTANCE))
        val makeArgs = mutableListOf(
            "--build",
            buildPath.fileName.toString(),
            "--config",
            cmakeConfig,
            "--target",
            target
        )

        if (parallelize) {
            makeArgs.addAll(listOf("--parallel", Runtime.getRuntime().availableProcessors().toString()))
        }

        return makeArgs
    }


    private fun createMakeCommand(buildPath: Path, parallelize: Boolean, target: String): LFCommand {
        val makeArgs = getMakeArgs(buildPath, parallelize, target)
        return commandFactory.createCommand("cmake", makeArgs, buildPath.parent)
    }

    private fun getCmakeArgs(
        buildPath: Path,
        outPath: Path,
        sourcesRoot: String? = null
    ) = cmakeArgs + listOf(
        // FIXME: The INSTALL parameters only relevant when we are targeting POSIX
        "-DCMAKE_INSTALL_PREFIX=${outPath.toUnixString()}",
        "-DCMAKE_INSTALL_BINDIR=$relativeBinDir",
        "-S",
        sourcesRoot ?: fileConfig.srcGenPath.toUnixString(),
        "-B",
        buildPath.fileName.toString()
    )

    private fun createCmakeCommand(
        buildPath: Path,
        outPath: Path,
        sourcesRoot: String? = null
    ): LFCommand {
        val cmd = commandFactory.createCommand(
            "cmake",
            getCmakeArgs(buildPath, outPath, sourcesRoot),
            buildPath.parent
        )
        return cmd
    }
}