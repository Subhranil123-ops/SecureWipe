param(
    [Parameter(Mandatory = $true)]
    [string]$RequestId,

    [Parameter(Mandatory = $true)]
    [string]$AuthToken,

    [Parameter(Mandatory = $true)]
    [string]$CertificatePath,

    [Parameter(Mandatory = $true)]
    [string]$AuditLogPath,

    [string]$BaseUrl = "https://securewipe-kuo0.onrender.com"
)

$ErrorActionPreference = "Stop"

# ============================================================
# HELPERS
# ============================================================

function Write-Step {
    param(
        [string]$Message
    )

    Write-Host ""
    Write-Host "============================================================"
    Write-Host $Message
    Write-Host "============================================================"
}

function Read-JsonFile {
    param(
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "File does not exist: $Path"
    }

    $content =
        Get-Content `
            -LiteralPath $Path `
            -Raw `
            -Encoding UTF8

    if (
        [string]::IsNullOrWhiteSpace(
            $content
        )
    ) {
        throw "File is empty: $Path"
    }

    try {
        return $content |
            ConvertFrom-Json
    }
    catch {
        throw "Invalid JSON file '$Path': $($_.Exception.Message)"
    }
}

function Get-RequiredProperty {
    param(
        [object]$Object,

        [string]$PropertyName
    )

    $property =
        $Object.PSObject.Properties[
            $PropertyName
        ]

    if ($null -eq $property) {
        throw "Required property '$PropertyName' is missing."
    }

    if (
        $null -eq $property.Value -or
        [string]::IsNullOrWhiteSpace(
            [string]$property.Value
        )
    ) {
        throw "Required property '$PropertyName' is empty."
    }

    return $property.Value
}

# ============================================================
# NORMALIZE BASE URL
# ============================================================

$BaseUrl =
    $BaseUrl.TrimEnd("/")

# ============================================================
# AUTHORIZATION
# ============================================================

$Headers = @{
    Authorization =
        "Bearer $AuthToken"

    "Content-Type" =
        "application/json"
}

# ============================================================
# STEP 1 - READ REAL CERTIFICATE
# ============================================================

Write-Step "STEP 1 - READING REAL CERTIFICATE"

$certificate =
    Read-JsonFile `
        -Path $CertificatePath

$certificateId =
    Get-RequiredProperty `
        -Object $certificate `
        -PropertyName "certificateId"

$certificateOperationId =
    Get-RequiredProperty `
        -Object $certificate `
        -PropertyName "operationId"

$certificateRequestId =
    Get-RequiredProperty `
        -Object $certificate `
        -PropertyName "requestId"

$workstationId =
    Get-RequiredProperty `
        -Object $certificate `
        -PropertyName "workstationId"

$certificateHash =
    Get-RequiredProperty `
        -Object $certificate `
        -PropertyName "certificateHash"

Write-Host "Certificate ID : $certificateId"
Write-Host "Operation ID   : $certificateOperationId"
Write-Host "Request ID     : $certificateRequestId"
Write-Host "Workstation ID : $workstationId"
Write-Host "SHA-256        : $certificateHash"

# ============================================================
# REQUEST ID CONSISTENCY CHECK
# ============================================================

if (
    $certificateRequestId -ne
    $RequestId
) {
    throw @"
Certificate requestId does not match the request supplied to the script.

Script Request ID:
$RequestId

Certificate Request ID:
$certificateRequestId
"@
}

# ============================================================
# CERTIFICATE SUCCESS CHECKS
# ============================================================

$status =
    [string]$certificate.status

$verificationStatus =
    [string]$certificate.verificationStatus

$verificationPerformed =
    [bool]$certificate.verificationPerformed

$verificationPassed =
    [bool]$certificate.verificationPassed

if (
    $status -ne "COMPLETED"
) {
    throw `
        "Refusing upload because certificate status is '$status', not COMPLETED."
}

if (
    $verificationStatus -ne "PASSED"
) {
    throw `
        "Refusing upload because verificationStatus is '$verificationStatus', not PASSED."
}

if (
    -not $verificationPerformed
) {
    throw `
        "Refusing upload because verificationPerformed is false."
}

if (
    -not $verificationPassed
) {
    throw `
        "Refusing upload because verificationPassed is false."
}

Write-Host `
    "[PASS] Certificate contains successful sanitization and verification evidence."

# ============================================================
# STEP 2 - READ REAL AUDIT JSONL
# ============================================================

Write-Step "STEP 2 - READING REAL AUDIT JSONL"

if (
    -not (
        Test-Path `
            -LiteralPath $AuditLogPath
    )
) {
    throw `
        "Audit log does not exist: $AuditLogPath"
}

$auditLog =
    Get-Content `
        -LiteralPath $AuditLogPath `
        -Raw `
        -Encoding UTF8

if (
    [string]::IsNullOrWhiteSpace(
        $auditLog
    )
) {
    throw "Audit log is empty."
}

$auditByteCount =
    [System.Text.Encoding]::UTF8.GetByteCount(
        $auditLog
    )

Write-Host "Audit Log: $AuditLogPath"
Write-Host "Audit Log Bytes: $auditByteCount"

# ============================================================
# STEP 3 - SUBMIT REAL SANITIZATION RESULT
# ============================================================

Write-Step "STEP 3 - SUBMITTING SANITIZATION RESULT"

$sanitizationResultBody = @{
    operationId =
        $certificateOperationId

    workstationId =
        $workstationId

    deviceId =
        [string]$certificate.deviceId

    model =
        [string]$certificate.model

    serialNumber =
        [string]$certificate.serialNumber

    capacityBytes =
        [long]$certificate.capacityBytes

    interfaceType =
        [string]$certificate.interfaceType

    method =
        [string]$certificate.method

    status =
        $status

    bytesProcessed =
        [long]$certificate.bytesProcessed

    operationDurationMs =
        [long]$certificate.operationDurationMs

    verificationStatus =
        $verificationStatus

    verificationPerformed =
        $verificationPerformed

    verificationPassed =
        $verificationPassed

    bytesVerified =
        [long]$certificate.bytesVerified

    verificationSamples =
        [int]$certificate.verificationSamples

    verificationMessage =
        [string]$certificate.verificationMessage

    nativeErrorCode =
        [int]$certificate.nativeErrorCode

    deviceReportedSuccess =
        [bool]$certificate.deviceReportedSuccess

    globalDataErased =
        [bool]$certificate.globalDataErased

    message =
        [string]$certificate.message
}

$sanitizationJson =
    $sanitizationResultBody |
    ConvertTo-Json `
        -Depth 10

$resultEndpoint =
    "$BaseUrl/api/sanitization-results/$RequestId"

Write-Host "POST $resultEndpoint"

$resultResponse =
    Invoke-RestMethod `
        -Method POST `
        -Uri $resultEndpoint `
        -Headers $Headers `
        -Body $sanitizationJson

Write-Host `
    "[PASS] Sanitization result accepted by backend."

if (
    $null -ne $resultResponse
) {
    $resultResponse |
        ConvertTo-Json `
            -Depth 20 |
        Write-Host
}

# ============================================================
# STEP 4 - SUBMIT REAL CERTIFICATE
# ============================================================

Write-Step "STEP 4 - SUBMITTING REAL CERTIFICATE"

$certificateEndpoint =
    "$BaseUrl/api/sanitization-certificates/$RequestId"

$certificateJson =
    $certificate |
    ConvertTo-Json `
        -Depth 20

Write-Host "POST $certificateEndpoint"

$certificateResponse =
    Invoke-RestMethod `
        -Method POST `
        -Uri $certificateEndpoint `
        -Headers $Headers `
        -Body $certificateJson

Write-Host `
    "[PASS] Certificate accepted by backend."

if (
    $null -ne $certificateResponse
) {
    $certificateResponse |
        ConvertTo-Json `
            -Depth 20 |
        Write-Host
}

# ============================================================
# STEP 5 - SUBMIT REAL AUDIT CHAIN
# ============================================================

Write-Step "STEP 5 - SUBMITTING REAL AUDIT CHAIN"

$auditEndpoint =
    "$BaseUrl/api/sanitization-audit/$RequestId"

$auditBody = @{
    operationId =
        $certificateOperationId

    certificateId =
        $certificateId

    workstationId =
        $workstationId

    auditLog =
        $auditLog
}

$auditJson =
    $auditBody |
    ConvertTo-Json `
        -Depth 20

Write-Host "POST $auditEndpoint"

$auditResponse =
    Invoke-RestMethod `
        -Method POST `
        -Uri $auditEndpoint `
        -Headers $Headers `
        -Body $auditJson

Write-Host `
    "[PASS] Audit chain accepted and cryptographically verified by backend."

if (
    $null -ne $auditResponse
) {
    $auditResponse |
        ConvertTo-Json `
            -Depth 20 |
        Write-Host
}

# ============================================================
# STEP 6 - DISPLAY RESULT
# ============================================================

Write-Step "EVIDENCE UPLOAD COMPLETED"

Write-Host ""
Write-Host "Certificate ID:"
Write-Host $certificateId

Write-Host ""
Write-Host "Operation ID:"
Write-Host $certificateOperationId

Write-Host ""
Write-Host "Request ID:"
Write-Host $RequestId

Write-Host ""
Write-Host "Workstation ID:"
Write-Host $workstationId

Write-Host ""
Write-Host "Certificate SHA-256:"
Write-Host $certificateHash

Write-Host ""
Write-Host "Certificate API:"
Write-Host `
    "$BaseUrl/api/sanitization-certificates/$certificateId"

Write-Host ""
Write-Host "Certificate Verification API:"
Write-Host `
    "$BaseUrl/api/sanitization-certificates/$certificateId/verify"

Write-Host ""
Write-Host "Audit Chain API:"
Write-Host `
    "$BaseUrl/api/sanitization-audit/$RequestId"

Write-Host ""
Write-Host "Audit Chain Verification API:"
Write-Host `
    "$BaseUrl/api/sanitization-audit/$RequestId/verify"

Write-Host ""
Write-Host "Frontend Certificate Page:"
Write-Host `
    "https://securewipe-web.onrender.com/workstation-employee/sanitization/certificate/$certificateId"

Write-Host ""

Write-Host "============================================================"
Write-Host "REAL SANITIZATION EVIDENCE UPLOAD COMPLETED"
Write-Host "============================================================"