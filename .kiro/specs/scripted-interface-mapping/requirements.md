# Requirements Document: Scripted Interface Mapping

## Introduction

This feature removes hardcoded interface definitions from the GoldSim-SWMM Bridge DLL by introducing a Python-based utility that automatically discovers the SWMM model structure and generates a static mapping file. The DLL will consume this mapping file at runtime, eliminating the need to rebuild the DLL when the SWMM model structure changes.

## Glossary

- **SWMM**: EPA Storm Water Management Model - hydraulic simulation engine
- **GoldSim**: Simulation software that interfaces with external DLLs
- **Bridge_DLL**: The GoldSim-SWMM Bridge DLL (GSswmm.dll) that connects GoldSim to SWMM
- **Mapping_File**: JSON file containing the discovered interface structure from a SWMM model
- **Parser_Script**: Python utility that reads SWMM .inp files and generates mapping files
- **INP_File**: SWMM input file with .inp extension containing model definition
- **External_Element**: GoldSim component that calls external DLL functions
- **DUMMY_Timeseries**: Special timeseries name indicating dynamic input from GoldSim
- **Interface_Index**: Zero-based array position in GoldSim's inargs or outargs arrays

## Requirements

### Requirement 1: Parse SWMM Input Files

**User Story:** As a developer, I want to parse SWMM .inp files to discover model structure, so that I can automatically determine the GoldSim interface requirements.

#### Acceptance Criteria

1. WHEN the Parser_Script receives a valid INP_File path, THE Parser_Script SHALL read and parse the file contents
2. WHEN the INP_File does not exist, THE Parser_Script SHALL return an error message indicating the file was not found
3. WHEN the INP_File contains invalid syntax, THE Parser_Script SHALL return an error message describing the parsing failure
4. WHEN parsing sections, THE Parser_Script SHALL ignore comment lines starting with semicolons
5. WHEN parsing sections, THE Parser_Script SHALL handle section headers enclosed in square brackets

### Requirement 2: Discover Input Interface Elements

**User Story:** As a developer, I want to automatically discover which SWMM elements require dynamic inputs from GoldSim, so that the correct number of inputs can be configured.

#### Acceptance Criteria

1. THE Parser_Script SHALL always assign elapsed time as the first input at Interface_Index 0
2. WHEN the [RAINGAGES] section contains gages using TIMESERIES named DUMMY, THE Parser_Script SHALL add each gage as a subsequent input
3. WHEN multiple rain gages use DUMMY_Timeseries, THE Parser_Script SHALL assign them sequential Interface_Index values starting from 1
4. WHEN the [RAINGAGES] section is missing, THE Parser_Script SHALL report only elapsed time as an input
5. WHEN a rain gage uses a timeseries other than DUMMY, THE Parser_Script SHALL exclude it from the input list

### Requirement 3: Discover Output Interface Elements

**User Story:** As a developer, I want to automatically discover which SWMM elements should report values to GoldSim, so that the correct number of outputs can be configured.

#### Acceptance Criteria

1. WHEN the [STORAGE] section contains entries, THE Parser_Script SHALL add each storage node as an output reporting current volume
2. WHEN the [OUTFALLS] section contains entries, THE Parser_Script SHALL add each outfall as an output reporting discharge flow rate
3. WHEN the [ORIFICES] section contains entries, THE Parser_Script SHALL add each orifice as an output reporting current flow rate
4. WHEN the [WEIRS] section contains entries, THE Parser_Script SHALL add each weir as an output reporting current flow rate
5. WHEN the [SUBCATCHMENTS] section contains entries, THE Parser_Script SHALL add each subcatchment as an output reporting runoff flow rate
6. THE Parser_Script SHALL assign output Interface_Index values in priority order: storage nodes, outfalls, orifices, weirs, then subcatchments
7. WHEN multiple elements exist within a section, THE Parser_Script SHALL preserve the order they appear in the INP_File

### Requirement 4: Generate Mapping File

**User Story:** As a developer, I want to generate a mapping file from the parsed SWMM model, so that the Bridge_DLL can dynamically configure its interface at runtime.

#### Acceptance Criteria

1. THE Parser_Script SHALL generate a Mapping_File in JSON format
2. THE Mapping_File SHALL have the base name "SwmmGoldSimBridge" with .json extension
3. THE Mapping_File SHALL contain the total count of input arguments
4. THE Mapping_File SHALL contain the total count of output arguments
5. THE Mapping_File SHALL contain an ordered list of input element names with their SWMM object types
6. THE Mapping_File SHALL contain an ordered list of output element names with their SWMM object types and value types
7. THE Mapping_File SHALL contain a version hash computed from the INP_File contents
8. WHEN the Mapping_File already exists, THE Parser_Script SHALL overwrite it with the new mapping

### Requirement 5: Validate Mapping File Synchronization

**User Story:** As a developer, I want to verify that the mapping file matches the SWMM model, so that I can detect when they become out of sync.

#### Acceptance Criteria

1. THE Parser_Script SHALL compute a hash from the INP_File contents
2. THE Parser_Script SHALL include the computed hash in the Mapping_File
3. THE Parser_Script SHALL exclude comments and whitespace when computing the hash
4. WHEN the INP_File is modified, THE Parser_Script SHALL generate a different hash value
5. THE Bridge_DLL SHALL read the hash from the Mapping_File during initialization

### Requirement 6: Update DLL to Report Dynamic Arguments

**User Story:** As a developer, I want the Bridge_DLL to report argument counts from the mapping file, so that GoldSim can validate the interface configuration.

#### Acceptance Criteria

1. WHEN XF_REP_ARGUMENTS is called, THE Bridge_DLL SHALL read the Mapping_File
2. WHEN the Mapping_File does not exist, THE Bridge_DLL SHALL return an error status
3. WHEN the Mapping_File is invalid JSON, THE Bridge_DLL SHALL return an error status with a descriptive message
4. WHEN the Mapping_File is valid, THE Bridge_DLL SHALL return the input count from the mapping
5. WHEN the Mapping_File is valid, THE Bridge_DLL SHALL return the output count from the mapping

### Requirement 7: Update DLL to Initialize from Mapping

**User Story:** As a developer, I want the Bridge_DLL to load element handles from the mapping file, so that it can interact with the correct SWMM objects.

#### Acceptance Criteria

1. WHEN XF_INITIALIZE is called, THE Bridge_DLL SHALL read the Mapping_File
2. WHEN the Mapping_File contains input elements, THE Bridge_DLL SHALL obtain SWMM API handles for each element
3. WHEN the Mapping_File contains output elements, THE Bridge_DLL SHALL obtain SWMM API handles for each element
4. WHEN an element name in the Mapping_File does not exist in the SWMM model, THE Bridge_DLL SHALL return an error status
5. WHEN the INP_File hash does not match the Mapping_File hash, THE Bridge_DLL SHALL log a warning message
6. THE Bridge_DLL SHALL store element handles for use during calculation

### Requirement 8: Update DLL to Calculate Using Mapping

**User Story:** As a developer, I want the Bridge_DLL to use the mapping to set inputs and retrieve outputs, so that it works with any SWMM model structure.

#### Acceptance Criteria

1. WHEN XF_CALCULATE is called, THE Bridge_DLL SHALL iterate through the input mapping to set values on SWMM elements
2. WHEN setting rain gage values, THE Bridge_DLL SHALL use swmm_setValue with the appropriate gage index
3. WHEN XF_CALCULATE is called, THE Bridge_DLL SHALL iterate through the output mapping to retrieve values from SWMM elements
4. WHEN retrieving output values, THE Bridge_DLL SHALL use swmm_getValue with the appropriate object type and index
5. THE Bridge_DLL SHALL populate the outargs array in the order specified by the Mapping_File
6. WHEN an element handle is invalid, THE Bridge_DLL SHALL return an error status

### Requirement 9: Provide Command-Line Interface

**User Story:** As a user, I want to run the parser script from the command line, so that I can generate mapping files for my SWMM models.

#### Acceptance Criteria

1. THE Parser_Script SHALL accept an INP_File path as a command-line argument
2. WHEN no command-line argument is provided, THE Parser_Script SHALL display usage instructions
3. WHEN the script completes successfully, THE Parser_Script SHALL print the output file path
4. WHEN the script completes successfully, THE Parser_Script SHALL print the discovered input and output counts
5. WHEN the script encounters an error, THE Parser_Script SHALL print an error message and exit with a non-zero status code

### Requirement 10: Handle Missing SWMM Sections

**User Story:** As a developer, I want the parser to handle SWMM models with missing sections gracefully, so that it works with minimal model configurations.

#### Acceptance Criteria

1. WHEN a SWMM section is not present in the INP_File, THE Parser_Script SHALL treat it as empty
2. WHEN no output sections are present, THE Parser_Script SHALL generate a Mapping_File with zero outputs
3. WHEN the [RAINGAGES] section is missing, THE Parser_Script SHALL generate a Mapping_File with only elapsed time as input
4. THE Parser_Script SHALL not fail when optional sections are absent
