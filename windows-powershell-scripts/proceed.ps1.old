[reflection.assembly]::LoadWithPartialName( "System.Windows.Forms")

function do_reboot
{
	$kaHnuK = gwmi win32_operatingsystem
	$kaHnuK.psbase.Scope.Options.EnablePrivileges = $true
	$kaHnuK.reboot()
}
function do_hibernate
{
	[Diagnostics.Process]::Start('shutdown','-h')
	$form.close()
}

$form= New-Object Windows.Forms.Form
$form.text = "Перезагрузка Windows"
$form.Size = New-Object Drawing.Point 325,130
$form.MinimumSize = $form.MaximumSize = $form.Size

$label = New-Object Windows.Forms.Label
$label.Size = New-Object Drawing.Point 300,15
$label.Font = $font
if ($args -eq "reboot") {
$label.Location = New-Object Drawing.Point 30,30 }
elseif ($args -eq "hibernate") {
$label.Location = New-Object Drawing.Point 45,30 
}
if ($args -eq "reboot") {
$label.text = "Вы действительно хотите перезагрузить Windows?"
} elseif ($args -eq "hibernate") {
$label.text = "Вы действительно хотите усыпить Windows?"
}

$button1 = New-Object Windows.Forms.Button
$button1.text = "Да"
$button1.Location = New-Object Drawing.Point 70,60
if ($args -eq "reboot") {
$button1.add_click({ do_reboot }) } elseif ($args -eq "hibernate")
{ $button1.add_click({ do_hibernate }) }

$button2 = New-Object Windows.Forms.Button
$button2.text = "Нет"
$button2.Location = New-Object Drawing.Point 170,60
$button2.add_click({ $form.close() })

$form.controls.AddRange(($label,$button1,$button2))

if (($args -eq "reboot") -or ($args -eq "hibernate"))
{ $form.ShowDialog() }