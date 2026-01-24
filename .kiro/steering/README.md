# Steering Documents

This directory contains steering documents that provide guidance to Kiro for working on this project.

## Available Steering Documents

### project-organization.md
**Inclusion**: Always

Guidelines for maintaining proper project organization:
- Directory structure and file placement rules
- Where to put test files, build artifacts, and source files
- Common mistakes to avoid
- Cleanup checklist before committing

**Key Rules:**
- All test files (`.cpp`, `.exe`, `.obj`, etc.) belong in `tests/` directory
- Test executables should be compiled with `/Fe tests\` flag
- Build artifacts should go in `x64/` or `GSswmm/` directories
- Keep root directory clean with only main source files and documentation

### build-process.md
**Inclusion**: Always

Comprehensive guide to the build process and workflows:
- Critical rule: Always copy GSswmm.dll to tests/ after building
- Build script descriptions and usage
- Complete build workflows for different scenarios
- Common build issues and solutions
- Build checklist before committing

**Key Rules:**
- After building `x64\Release\GSswmm.dll`, ALWAYS copy to `tests\GSswmm.dll`
- Use `scripts\release.bat` for automated build-test cycles
- Rebuild test executables after interface or version changes
- Verify DLL versions match before running tests

## How Steering Documents Work

Steering documents can be configured with different inclusion modes:

- **Always included**: Loaded in every Kiro session (use for critical guidelines)
- **Conditional**: Loaded when specific files are accessed (use for file-specific rules)
- **Manual**: Loaded only when explicitly referenced with `#` in chat (use for optional guidance)

## Adding New Steering Documents

When creating new steering documents:

1. Create a `.md` file in this directory
2. Add front-matter to specify inclusion mode:
   ```markdown
   ---
   inclusion: always
   ---
   ```
3. Write clear, actionable guidelines
4. Include examples of correct and incorrect usage
5. Update this README with a description

## Best Practices

- Keep steering documents focused on specific topics
- Use concrete examples (✅ CORRECT / ❌ WRONG)
- Include checklists for verification
- Reference specific requirements or conventions
- Update documents as project evolves
