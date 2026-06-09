Name:           sysmon
Version:        4.1.0
Release:        1%{?dist}
Summary:        Small system monitoring daemon
License:        MIT
URL:            https://github.com/abmcmanu/sysmon
Source0:        %{url}/archive/v%{version}/sysmon-%{version}.tar.gz
BuildRequires:  gcc make
BuildRequires:  systemd-rpm-macros

%description
sysmond samples cpu, memory, load, disks and plugin metrics and serves
them over a unix socket, a Prometheus endpoint and a web dashboard.

%prep
%autosetup

%build
%make_build CFLAGS="%{optflags}"

%check
make test

%install
%make_install PREFIX=%{_prefix}
install -Dm644 contrib/sysmond.service %{buildroot}%{_unitdir}/sysmond.service
install -Dm644 contrib/sysmond.conf %{buildroot}%{_sysconfdir}/sysmond.conf

%post
%systemd_post sysmond.service

%preun
%systemd_preun sysmond.service

%files
%license LICENSE
%doc README.md CHANGELOG.md
%config(noreplace) %{_sysconfdir}/sysmond.conf
%{_bindir}/sysmond
%{_bindir}/sysmonctl
%{_prefix}/lib/sysmon/
%{_mandir}/man8/sysmond.8*
%{_datadir}/bash-completion/completions/sysmonctl
%{_datadir}/zsh/site-functions/_sysmonctl
%{_datadir}/fish/vendor_completions.d/sysmonctl.fish
%{_unitdir}/sysmond.service
