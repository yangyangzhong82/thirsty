add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
add_requires("levilamina 26.51.5", {configs = {target_type = get_config("target_type")}})

add_requires("levibuildscript")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("thirsty")
    set_version("0.2.1")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "0.2.1"})
    if is_plat("windows") then
        add_defines("NOMINMAX", "UNICODE", "_HAS_CXX23=1")
        set_exceptions("none") -- To avoid conflicts with /EHa.
        add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            {tools = {"clang_cl"}}
        )
        set_toolchains("clang-cl")
    end
    add_packages("levilamina")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")
    set_targetdir("build/$(plat)/$(arch)/$(mode)/" .. (get_config("target_type") or "server"))
    set_objectdir("build/.objs/" .. (get_config("target_type") or "server"))
    if is_config("target_type", "client") then
        add_includedirs("src-client")
        add_headerfiles("src-client/**.h")
        add_files("src-client/**.cpp")
    end
    after_build(function(target)
        local outputdir = path.join(os.projectdir(), "bin", target:name())
        for _, kind in ipairs({"behavior_packs", "resource_packs"}) do
            local destination = path.join(outputdir, kind)
            os.mkdir(destination)
            os.cp(path.join(os.projectdir(), "packs", kind, "thirsty"), destination)
        end
    end)
