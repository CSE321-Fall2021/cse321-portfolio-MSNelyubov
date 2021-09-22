<#*****************************************************************************
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   09/22/2021
*   Last Modified:  09/22/2021
*   Purpose:        This powershell script copies all content from the working
*                     project directory into the CSE321 Git Repository under
*                     the Project 1 subfolder.
*
*   Functions:
*
*   Associated Assignment:  CSE 321 Project 1, Fall 2021
*
*   Inputs:         None
*
*   Outputs:        Confirmation printed via Verbose print output.
*
*   Constraints:    Copying all existing repository items will overwrite exist-
*                     ing objects in the local project directory.  Be sure that
*                     no data will be lost before pushing the working copy into
*                     the version control section.
*
*   External Sources:
*
******************************************************************************#>

$ignoreList = Get-Content -Path $PSScriptRoot\.gitignore | where {$_.Length -gt 0}

$copyItemList = @()



ls $PSScriptRoot | foreach {
    $fileName = $_.FullName
    $skipFile = $false
    $ignoreList | foreach {
        $shortName = $fileName.Replace($PSScriptRoot,"")
        if($shortName.Contains($_)){
            $skipFile = $true
            Write-Host "Ignoring $shortName" -ForegroundColor Red
        }
    }
    if(-not $skipFile){
        Write-Host "Adding $shortName to copy list" -ForegroundColor Green
        $copyItemList += $fileName
    }
}

$gitPathRoot = '~\Documents\GitHub\cse321-portfolio-MSNelyubov\Project 1'

$copyItemList | foreach {
    Copy-Item -Path $_ -Destination ($_.Replace($PSScriptRoot, $gitPathRoot)) -Verbose
}

